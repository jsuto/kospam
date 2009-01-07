/*
 * qcache.c, 2009.01.05, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <mysql.h>
#include <pthread.h>
#include <clapf.h>
#include "cache.h"
#include "buffer.h"


#define QCACHE_PROGNAME "Qcache"
#define ALARM_TIME 300

extern char *optarg;
extern int optind;

typedef struct {
   int sockfd;
   pthread_t thread;
} qconn;


int sd;
int nconn = 0;
int toggle = 0;
char *configfile = CONFIG_FILE;
struct __config cfg;
struct cache *hash_table1[MAX_CACHE_HASH], *hash_table2[MAX_CACHE_HASH], **m;
struct timezone tz;
struct timeval tv1, tv2;

pthread_mutex_t __lock;
pthread_attr_t attr;
int __num_threads, listener;


float calc_spamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x);


/*
 * release everything before we exit
 */

void clean_exit(){
   if(sd != -1)
      close(sd);

   clearcache(hash_table1);
   clearcache(hash_table2);

   pthread_attr_destroy(&attr);

   syslog(LOG_PRIORITY, "%s has been terminated", QCACHE_PROGNAME);
   exit(1);
}


/*
 * exit with an error string
 */

void fatal(char *s){
   syslog(LOG_PRIORITY, "%s\n", s);
   clean_exit();
}


/*
 * reload configuration
 */

void reload_config(){
   cfg = read_config(configfile);
   syslog(LOG_PRIORITY, "reloaded config: %s", configfile);
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


void *process_connection(void *ptr){
   qconn *QC = (qconn*)ptr;
   int j, n;
   char buf[PKT_SIZE];
   struct x_token *x;

   while((n = recv(QC->sockfd, buf, sizeof buf, 0)) > 0){
      if(n > 0){
          for(j=0; j<NETWORK_SEGMENT_SIZE; j++){
             x = (struct x_token*)(buf + j*sizeof(struct x_token));
             if(x->token > 0) x->spaminess = spamcache(m, x->token);
          }
          send(QC->sockfd, buf, PKT_SIZE, 0);
      }
   }

   close(QC->sockfd);

   decrement_thread_count();
   pthread_exit(0);

   return 0;
}


int load_all_tokens(struct cache *xhash[]){
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;
   char stmt[SMALLBUFSIZE];
   float Nham=0, Nspam=0, nh=0, ns=0;
   unsigned long long key;
   float spamicity;
   int ntokens = 0;

   mysql_init(&mysql);
   mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0) == 0){
      syslog(LOG_PRIORITY, "%s", ERR_MYSQL_CONNECT);
      return 0;
   }

   snprintf(stmt, SMALLBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);

   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            Nham += atof(row[0]);
            Nspam += atof(row[1]);
         }
         mysql_free_result(res);
      }
   }

   snprintf(stmt, SMALLBUFSIZE-1, "SELECT token, nham, nspam FROM %s WHERE uid=0", SQL_TOKEN_TABLE);

   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            key = atoll(row[0]);
            nh = atof(row[1]);
            ns = atof(row[2]);
            spamicity = calc_spamicity(Nham, Nspam, nh, ns, cfg.rob_s, cfg.rob_x);
            ntokens++;
            addcache(xhash, key, spamicity);
         }
         mysql_free_result(res);
      }
   }

   mysql_close(&mysql);

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "we have %.0f good and %.0f bad messages with %d tokens\n", Nham, Nspam, ntokens);

   return 1;
}


int flush_dirty(struct cache *xhash[]){
   char buf[SMALLBUFSIZE];
   unsigned long now;
   time_t cclock;
   buffer *query;
   int n=0, total=0;
   MYSQL mysql;

   time(&cclock);
   now = cclock;

   snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET timestamp=%ld WHERE token in (", SQL_TOKEN_TABLE, now);

   mysql_init(&mysql);
   mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0) == 0){
      syslog(LOG_PRIORITY, "%s", ERR_MYSQL_CONNECT);
      return 0;
   }

   do {
      query = buffer_create(NULL);

      if(query){

         buffer_cat(query, buf);

         n = flush_dirty_cache(xhash, query);
         total += n;

         buffer_cat(query, "0) AND uid=0");

         if(n > 0) mysql_real_query(&mysql, query->data, strlen(query->data));

         buffer_destroy(query);
      }
   } while(n);

   if(total > 0) syslog(LOG_PRIORITY, "updated %d tokens", total);

   return 1;
}


void swap_hash_table(){
   int rc = 0;

   if(toggle == 0){
      gettimeofday(&tv1, &tz);
      rc = load_all_tokens(hash_table2);
      gettimeofday(&tv2, &tz);

      if(rc == 1){
         m = &hash_table2[0];
         toggle = 1;
         flush_dirty(hash_table1);
         clearcache(hash_table1);
      }
      else clearcache(hash_table2);
   }
   else {
      gettimeofday(&tv1, &tz);
      rc = load_all_tokens(hash_table1);
      gettimeofday(&tv2, &tz);

      if(rc == 1){
         m = &hash_table1[0];
         toggle = 0;
         flush_dirty(hash_table2);
         clearcache(hash_table2);
      }
      else clearcache(hash_table1);
   }

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "reading tokens from database in %ld [ms]", tvdiff(tv2, tv1)/1000);

   alarm(ALARM_TIME);
}


int main(int argc, char **argv){
   int i, j, new_sd=-1, yes=1, daemonise=0;
   unsigned int clen;
   struct sockaddr_in client_addr, serv_addr;
   struct in_addr addr;
   fd_set master;
   fd_set read_fds;
   int fdmax;
   struct x_token *x;
   struct timeval tv;
   qconn *QC;

   while((i = getopt(argc, argv, "c:u:g:dVh")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'd' :
                    daemonise = 1;
                    break;

         case 'V' :
                    __fatal(QCACHE_PROGNAME " " VERSION);
                    break;

         case 'h' :
         default  : 
                    __fatal("usage: " QCACHE_PROGNAME " -c <config file> -d");
       }
   }

   initcache(hash_table1);
   initcache(hash_table2);


   (void) openlog(QCACHE_PROGNAME, LOG_PID, LOG_MAIL);

   signal(SIGINT, clean_exit);
   signal(SIGQUIT, clean_exit);
   signal(SIGKILL, clean_exit);
   signal(SIGTERM, clean_exit);
   signal(SIGHUP, reload_config);
   signal(SIGALRM, swap_hash_table);

   reload_config();

   if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      fatal(ERR_OPEN_SOCKET);

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(MY_PORT);
   inet_aton(cfg.listen_addr, &addr);
   serv_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(serv_addr.sin_zero), 8);

   if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
       fatal(ERR_SET_SOCK_OPT);

   if(bind(sd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
       fatal(ERR_BIND_TO_PORT);

   if(listen(sd, cfg.backlog) == -1)
       fatal(ERR_LISTEN);


   syslog(LOG_PRIORITY, "%s %s starting", QCACHE_PROGNAME, VERSION);

   clen = sizeof(client_addr);


   /* go to the background */
   if(daemonise == 1) daemon(1, 0);


   swap_hash_table();


   FD_ZERO(&master);
   FD_ZERO(&read_fds);

   FD_SET(sd, &master);
   fdmax = sd;


   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   __num_threads=0;


   /* main accept loop */

   tv.tv_sec = 10;
   tv.tv_usec = 0;

   for(;;){
    read_fds = master; // copy it
   
    if(select(fdmax+1, &read_fds, NULL, NULL, &tv) > 0){


      /* run through the existing connections looking for data to read */

      for(i=0; i <= fdmax; i++){
         if(FD_ISSET(i, &read_fds)){ // we got one!!

            /* handle new connections */

            if(i == sd){
               if((new_sd = accept(sd, (struct sockaddr *)&client_addr, &clen)) == -1) continue;

               fcntl(new_sd, F_SETFL, O_RDWR);
               setsockopt(new_sd, SOL_SOCKET, TCP_NODELAY, &yes, sizeof(int));

               QC = malloc(sizeof(qconn));
               if(QC == NULL){
                  close(new_sd);
                  continue;
               } else {
                  QC->sockfd = new_sd;
                  increment_thread_count();
                  if(pthread_create(&QC->thread, &attr, process_connection, (void *) QC)){
                     decrement_thread_count();
                     close(QC->sockfd);
                     free(QC);
                     continue;
                  }
               }
            }


         }
      }

    }
   }

   return 0;
}
