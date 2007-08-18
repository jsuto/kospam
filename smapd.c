/*
 * smapd.c, 2007.08.13, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "misc.h"
#include "errmsg.h"
#include "smapd.h"
#include "bayes.h"
#include "config.h"

#ifdef HAVE_MYSQL
  #include <mysql.h>
#endif
#ifdef HAVE_SQLITE3
  #include <sqlite3.h>
#endif

extern char *optarg;
extern int optind;


char *configfile = CONFIG_FILE;
struct __config cfg;

SSL_CTX *ctx = NULL;
SSL_METHOD *meth = NULL;


typedef struct {
   int sockfd;
   pthread_t thread;
} qconn;


struct timezone tz;
pthread_mutex_t __lock;
pthread_attr_t attr;
int __num_threads, listener;


/*
 * shutdown cache
 */

void smap_down(){

   close(listener);
   pthread_attr_destroy(&attr);

   SSL_CTX_free(ctx);

   syslog(LOG_PRIORITY, "shutting down...");

   exit(1);
}


/*
 * exit with a message
 */

void fatal(char *s){
   printf("%s\n", s);
   smap_down();
}


/*
 * increment thread counter
 */

void increment_thread_count(void){
   pthread_mutex_lock(&__lock);
   __num_threads++;
   pthread_mutex_unlock(&__lock);
}


/*
 * decrement thread counter
 */

void decrement_thread_count(void){
   pthread_mutex_lock(&__lock);
   __num_threads--;
   pthread_mutex_unlock(&__lock);
}


/*
 * SSL aware read
 */

int read1(int sd, char *buf, int use_ssl, SSL *ssl){
   fd_set fds;
   int err;
   struct timeval tv;

   memset(buf, 0, MAXBUFSIZE);

   FD_ZERO(&fds);
   FD_SET(sd, &fds);

   tv.tv_sec = SMAP_TIMEOUT;
   tv.tv_usec = SMAP_TIMEOUT_USEC;

   err = select(sd+1, &fds, NULL, NULL, &tv);
   if(err == 0) return -2; // timeout!
   if(err == -1) return -1; // error

   if(use_ssl == 1)
      err = SSL_read(ssl, buf, MAXBUFSIZE-1);
   else
      err = recv(sd, buf, MAXBUFSIZE-1, 0);

   return err;
}


/*
 * process a connection
 */

void *process_connection(void *ptr){
   qconn *QC = (qconn*)ptr;
   struct timeval tv_start, tv_stop;
   unsigned long ts;
   struct timezone tz;
   struct timeval tv;
   char *p, buf[MAXBUFSIZE], cmdbuf[MAXBUFSIZE], puf[2*MAXBUFSIZE];
   int n, auth_ok=0, tot_len=0, fd, prevlen=0;
   double spaminess=DEFAULT_SPAMICITY;
   SSL *ssl = NULL;
   struct session_data sdata;
   struct _state state;

   memset(buf, 0, MAXBUFSIZE);
   memset(cmdbuf, 0, MAXBUFSIZE);


   ssl = SSL_new(ctx);
   if(ssl == NULL) goto CLOSE;

   if(SSL_set_fd(ssl, QC->sockfd) == 0) goto CLOSE;
   if(SSL_accept(ssl) <= 0) goto CLOSE;


#ifdef HAVE_MYSQL
   MYSQL mysql;

   mysql_init(&mysql);
   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      SSL_write(ssl, RESP_SMAPD_ERR_BANNER, strlen(RESP_SMAPD_ERR_BANNER));
      goto CLOSE;
   }

#endif
#ifdef HAVE_SQLITE3
   sqlite3 *db;
   int rc;

   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc){
      SSL_write(ssl, RESP_SMAPD_ERR_BANNER, strlen(RESP_SMAPD_ERR_BANNER));
      goto CLOSE;
   }
#endif

   SSL_write(ssl, RESP_SMAPD_OK_BANNER, strlen(RESP_SMAPD_OK_BANNER));

   gettimeofday(&tv_start, &tz);


   /* the first command must be a 'HELO username key' */

   //n = SSL_read(ssl, buf, MAXBUFSIZE-1);
   if(read1(QC->sockfd, buf, 1, ssl) <= 0) goto CLOSE;

   if(strncmp(buf, "HELO ", 5) == 0){
      p = strchr(&buf[5], ' ');
      if(p){
         *p = '\0';
         p++;
         trim(p);

         snprintf(puf, MAXBUFSIZE-1, "SELECT uid, expires FROM %s WHERE email='%s' AND serial='%s'", SQL_SMAP_KEY_TABLE, &buf[5], p);

         /* authenticate user */

         #ifdef HAVE_MYSQL
            MYSQL_RES *res;
            MYSQL_ROW row;

            if(mysql_real_query(&mysql, puf, strlen(puf)) == 0){
               res = mysql_store_result(&mysql);
               if(res){
                  row = mysql_fetch_row(res);
                  mysql_free_result(res);

                  if(row){
                     sdata.uid = atol(row[0]);
                     ts = atol(row[1]);
                     gettimeofday(&tv, &tz);

                     if(ts >= tv.tv_sec) auth_ok = 1;
                  }
               }
            }
         #endif

      } 
   }


   if(auth_ok == 0){
      SSL_write(ssl, RESP_SMAPD_ERR_AUTH_FAILED, strlen(RESP_SMAPD_ERR_AUTH_FAILED));
      goto CLOSE;
   }

   SSL_write(ssl, RESP_SMAPD_OK, strlen(RESP_SMAPD_OK));

   sdata.num_of_rcpt_to = 1;
   make_rnd_string(&(sdata.ttmpfile[0]));

   fd = open(sdata.ttmpfile, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
   if(fd == -1){
      SSL_write(ssl, RESP_SMAPD_ERR, strlen(RESP_SMAPD_ERR));
      goto CLOSE;
   }

   //fprintf(stderr, "processing: %s\n", sdata.ttmpfile);

   /* read the message until we got the trailing period */

   //while((n = SSL_read(ssl, buf, MAXBUFSIZE-1))){
   while((n = read1(QC->sockfd, buf, 1, ssl))){
      write(fd, buf, n);

      tot_len += n;

      memset(puf, 0, 2*MAXBUFSIZE);
      memcpy(puf, cmdbuf, prevlen);
      memcpy(puf+prevlen, buf, n);

      // save buffer
      memcpy(cmdbuf, buf, n);

      if(puf[prevlen+n-5] == '\r' && puf[prevlen+n-4] == '\n' && puf[prevlen+n-3] == '.' && puf[prevlen+n-2] == '\r' && puf[prevlen+n-1] == '\n') break;

      prevlen = n;

      memset(buf, 0, MAXBUFSIZE);
   }
   close(fd);

   /* if we have to check the message */

   if(tot_len <= cfg.max_message_size_to_filter){
      state = parse_message(sdata.ttmpfile, cfg);
      spaminess = bayes_file(mysql, sdata.ttmpfile, state, sdata, cfg);
   }

   unlink(sdata.ttmpfile);

   //fprintf(stderr, "done: %s\n", sdata.ttmpfile);

   /* send answer back */

   if(spaminess >= cfg.spam_overall_limit)
      snprintf(buf, MAXBUFSIZE-1, "+OK SPAM %.4f %s\r\n", spaminess, sdata.ttmpfile);
   else
      snprintf(buf, MAXBUFSIZE-1, "+OK HAM %.4f %s\r\n", spaminess, sdata.ttmpfile);

   syslog(LOG_PRIORITY, "uid: %ld %s", sdata.uid, buf);

   SSL_write(ssl, buf, strlen(buf));

   gettimeofday(&tv_stop, &tz);


CLOSE:

#ifdef HAVE_MYSQL
   mysql_close(&mysql);
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(db);
#endif

   SSL_free(ssl);

   close(QC->sockfd);

   decrement_thread_count();
   pthread_exit(0);

   return 0;
}


int main(int argc, char **argv){
   int i, fdmax, newfd, addrlen, yes=1;
   struct sockaddr_in remote_addr;
   struct sockaddr_in local_addr;
   struct in_addr addr;
   struct timeval tv;
   qconn *QC;
   fd_set master, read_fds;


   while((i = getopt(argc, argv, "c:")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         default  : 
                    break;
       }
   }


   signal(SIGINT, smap_down);
   signal(SIGQUIT, smap_down);
   signal(SIGKILL, smap_down);
   signal(SIGTERM, smap_down);


   cfg = read_config(configfile);


   /* initialize */

   (void) openlog(SMAPD_PROGRAM_NAME, LOG_PID, LOG_MAIL);

   if(chdir(cfg.workdir)) fatal("cannot chdir()");

   /* init SSL stuff */

   SSL_load_error_strings();
   SSLeay_add_ssl_algorithms();
   meth = SSLv3_server_method();

   ctx = SSL_CTX_new(meth);
   if(ctx == NULL) smap_down();

   if(SSL_CTX_use_certificate_file(ctx, cfg.ssl_cert_file, SSL_FILETYPE_PEM) <= 0)
      fatal("cannot read certificate file");

   if(SSL_CTX_use_PrivateKey_file(ctx, cfg.ssl_key_file, SSL_FILETYPE_PEM) <= 0)
      fatal("cannot read private key file");

   if(!SSL_CTX_check_private_key(ctx))
      fatal("Private key does not match the certificate public key");


   FD_ZERO(&master);
   FD_ZERO(&read_fds);

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   __num_threads=0;


   /* create a listener socket */

   if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      fatal(ERR_OPEN_SOCKET);

   local_addr.sin_family = AF_INET;
   local_addr.sin_port = htons(SMAPD_PORT);
   inet_aton(cfg.listen_addr, &addr);
   local_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(local_addr.sin_zero), 8);

   if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      fatal(ERR_SET_SOCK_OPT);

   if(bind(listener, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1)
      fatal(ERR_BIND_TO_PORT);


   if(listen(listener, cfg.backlog) == -1)
      fatal(ERR_LISTEN);

   FD_SET(listener, &master);
   fdmax = listener;

   syslog(LOG_PRIORITY, "started");
 
   for(;;) {
      read_fds = master;

      tv.tv_sec = 2;
      tv.tv_usec = 0;


      if(select(fdmax+1, &read_fds, NULL, NULL, &tv) > 0){

         for(i=0;i<=fdmax;i++) {
            if(FD_ISSET(i, &read_fds)){

               /* handle connections */

               if(i == listener){
                  addrlen = sizeof(remote_addr);

                  if((newfd = accept(listener, (struct sockaddr *)&remote_addr, (socklen_t *) &addrlen)) == -1){
                     fprintf(stderr, "daemon error accept\n");
                     continue;
                  }
                  fcntl(newfd, F_SETFL, O_RDWR);
                  setsockopt(newfd, SOL_SOCKET, TCP_NODELAY, &yes, sizeof(int));

                  if(__num_threads >= MAX_THREADS){
                     send(newfd, RESP_SMAPD_ERR_BANNER, strlen(RESP_SMAPD_ERR_BANNER), 0);
                     goto DEFERRED;
                  }

                  QC = malloc(sizeof(qconn));
                  if(QC == NULL) goto DEFERRED;

                  increment_thread_count();
                  QC->sockfd = newfd;


                  if(pthread_create(&QC->thread, &attr, process_connection, (void *)QC)){
                     decrement_thread_count();

                  DEFERRED:
                     close(newfd);
                     continue;
                  }

               }
            }
         }
      }
   }



   return 0;
}
