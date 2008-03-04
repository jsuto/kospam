/*
 * clapfstore.c, 2008.03.04, SJ
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
#include "clapfstore.h"
#include "ssl_util.h"
#include "config.h"
#include "cfg.h"

extern char *optarg;
extern int optind;


char *configfile = CONFIG_FILE;
struct __config cfg;

SSL_CTX *ctx = NULL;
SSL_METHOD *meth = NULL;


typedef struct {
   int sockfd, use_ssl;
   pthread_t thread;
} qconn;


struct timezone tz;
pthread_mutex_t __lock;
pthread_attr_t attr;
int __num_threads, listener;


/*
 * shutdown cache
 */

void store_down(){

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
   store_down();
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
 * store the given message
 */

int store_message(char *filename, int sd, int use_ssl, SSL *ssl){
   int n, fd, tot_len=0, prevlen=0, rc=0;
   char buf[MAXBUFSIZE], cmdbuf[MAXBUFSIZE], puf[2*MAXBUFSIZE];

   fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
   if(fd == -1)
      return rc;

   /* read the message until we got the trailing period */

   while((n = read1(sd, buf, use_ssl, ssl))){
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

   if(fsync(fd) == 0) rc = 1;
   close(fd);

   return rc;
}


/*
 * retrieve the given message
 */

int retr_message(char *filename, int sd, int use_ssl, SSL *ssl){
   int n, fd;
   char buf[MAXBUFSIZE];

   fd = open(filename, O_RDONLY);
   if(fd == -1)
      return 0;

   write1(sd, RESP_CLAPFSTORE_OK, use_ssl, ssl);

   while((n = read(fd, buf, MAXBUFSIZE-1))){
      write1(sd, buf, use_ssl, ssl);
   }

   close(fd);

   if(strstr(buf, "\r\n.\r\n") == NULL)
      write1(sd, "\r\n.\r\n", use_ssl, ssl);

   return 1;
}


/*
 * process a connection
 */

void *process_connection(void *ptr){
   qconn *QC = (qconn*)ptr;
   char *p, *q, buf[MAXBUFSIZE];
   char user[SMALLBUFSIZE], sig[SMALLBUFSIZE], secret[SMALLBUFSIZE];
   int rc, ham_or_spam=0, cmd=0, auth_ok=0;
   SSL *ssl = NULL;
   struct stat st;

   memset(buf, 0, MAXBUFSIZE);

   memset(user, 0, SMALLBUFSIZE);
   memset(sig, 0, SMALLBUFSIZE);
   memset(secret, 0, SMALLBUFSIZE);

   if(QC->use_ssl == 1){
      ssl = SSL_new(ctx);
      if(ssl == NULL) goto CLOSE;

      if(SSL_set_fd(ssl, QC->sockfd) == 0) goto CLOSE;
      if(SSL_accept(ssl) <= 0) goto CLOSE;
   }

   write1(QC->sockfd, RESP_CLAPFSTORE_OK_BANNER, QC->use_ssl, ssl);

   if(read1(QC->sockfd, buf, QC->use_ssl, ssl) <= 0) goto CLOSE;

   /* store the given message */
   if(strncmp(buf, "STOR ", 5) == 0){
      cmd = CMD_STOR;
   }

   /* retrieve the given message */
   else if(strncmp(buf, "RETR ", 5) == 0){
      cmd = CMD_RETR;
   }

   /* invalid command, send error back, then quit */
   if(cmd == 0){
      write1(QC->sockfd, RESP_CLAPFSTORE_ERR, QC->use_ssl, ssl);
      goto CLOSE;
   }

   /* extract user, signature, ham_or_spam and secret */

   q = &buf[5];

   p = strchr(q, ' ');
   if(p){
      *p = '\0';
      snprintf(user, SMALLBUFSIZE-1, "%s", q);
      q = p+1;

      p = strchr(q, ' ');
      if(p){
         *p = '\0';
         snprintf(sig, SMALLBUFSIZE-1, "%s", q);
         q = p+1;

         p = strchr(q, ' ');
         if(p){
            *p = '\0';
            ham_or_spam = atoi(q);
            q = p+1;

            snprintf(secret, SMALLBUFSIZE-1, "%s", q);
            trim(secret);

            if(strcmp(secret, cfg.store_secret) == 0){
               auth_ok = 1;
            }
         }
      }

   }

   syslog(LOG_PRIORITY, "%s %s %d", user, sig, ham_or_spam);

   if(auth_ok == 0){
      write1(QC->sockfd, RESP_CLAPFSTORE_ERR, QC->use_ssl, ssl);
      goto CLOSE;
   }


   /* determine path */

   snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s", cfg.chrootdir, USER_QUEUE_DIR, user[0], user);
   if(stat(buf, &st) != 0){
      mkdir(buf, 0700);
      if(stat(buf, &st) != 0){
         write1(QC->sockfd, RESP_CLAPFSTORE_ERR, QC->use_ssl, ssl);
         goto CLOSE;
      }
   }

   snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s/%s.%d", cfg.chrootdir, USER_QUEUE_DIR, user[0], user, sig, ham_or_spam);


   /* store message */

   if(cmd == CMD_STOR){
      write1(QC->sockfd, RESP_CLAPFSTORE_OK, QC->use_ssl, ssl);

      rc = store_message(buf, QC->sockfd, QC->use_ssl, ssl);

      if(rc == 0) write1(QC->sockfd, RESP_CLAPFSTORE_ERR, QC->use_ssl, ssl);
      else write1(QC->sockfd, RESP_CLAPFSTORE_OK, QC->use_ssl, ssl);
   }

   /* retrieve message */
   else if(cmd == CMD_RETR){
      rc = retr_message(buf, QC->sockfd, QC->use_ssl, ssl);
      if(rc == 0) write1(QC->sockfd, RESP_CLAPFSTORE_ERR, QC->use_ssl, ssl);
   }

   else write1(QC->sockfd, RESP_CLAPFSTORE_ERR, QC->use_ssl, ssl);

CLOSE:

   if(QC->use_ssl == 1)
      SSL_free(ssl);

   close(QC->sockfd);

   decrement_thread_count();
   pthread_exit(0);

   return 0;
}


int main(int argc, char **argv){
   int i, fdmax, newfd, addrlen, yes=1, daemonise=0;
   struct sockaddr_in remote_addr;
   struct sockaddr_in local_addr;
   struct in_addr addr;
   struct timeval tv;
   qconn *QC;
   fd_set master, read_fds;


   while((i = getopt(argc, argv, "c:d")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'd' :
                    daemonise = 1;
                    break;

         default  : 
                    break;
       }
   }


   signal(SIGINT, store_down);
   signal(SIGQUIT, store_down);
   signal(SIGKILL, store_down);
   signal(SIGTERM, store_down);


   cfg = read_config(configfile);


   /* initialize */

   (void) openlog(CLAPFSTORE_PROGRAM_NAME, LOG_PID, LOG_MAIL);

   if(chdir(cfg.workdir)) fatal("cannot chdir()");

   /* init SSL stuff */

#ifdef HAVE_SSL
   SSL_load_error_strings();
   SSLeay_add_ssl_algorithms();
   meth = TLSv1_server_method();

   ctx = SSL_CTX_new(meth);
   if(ctx == NULL) store_down();

   if(SSL_CTX_use_certificate_file(ctx, cfg.ssl_cert_file, SSL_FILETYPE_PEM) <= 0)
      fatal("cannot read certificate file");

   if(SSL_CTX_use_PrivateKey_file(ctx, cfg.ssl_key_file, SSL_FILETYPE_PEM) <= 0)
      fatal("cannot read private key file");

   if(!SSL_CTX_check_private_key(ctx))
      fatal("Private key does not match the certificate public key");
#endif

   FD_ZERO(&master);
   FD_ZERO(&read_fds);

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   __num_threads=0;


   /* create a listener socket */

   if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      fatal(ERR_OPEN_SOCKET);

   local_addr.sin_family = AF_INET;
   local_addr.sin_port = htons(cfg.store_port);
   inet_aton(cfg.store_addr, &addr);
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

   if(daemonise == 1) daemon(1, 0);

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
                     send(newfd, RESP_CLAPFSTORE_ERR_BANNER, strlen(RESP_CLAPFSTORE_ERR_BANNER), 0);
                     goto DEFERRED;
                  }

                  QC = malloc(sizeof(qconn));
                  if(QC == NULL) goto DEFERRED;

                  increment_thread_count();
                  QC->sockfd = newfd;
               #ifdef HAVE_SSL   
                  QC->use_ssl = 1;
               #else
                  QC->use_ssl = 0;
               #endif

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
