/*
 * qs.c, 2007.07.20, SJ
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
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "misc.h"
#include "errmsg.h"
#include "qcache.h"
#include "cfg.h"
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



typedef struct {
   int sockfd;
   pthread_t thread;
} qconn;


struct qcache *Q[MAXHASH];
struct token_entry RES;
struct timezone tz;
pthread_mutex_t __lock;
pthread_attr_t attr;
int __num_threads, listener;


/*
 * shutdown cache
 */

void cache_down(){
   clearhash(Q);

   unlink(QCACHE_SOCKET);

   close(listener);
   pthread_attr_destroy(&attr);

   syslog(LOG_PRIORITY, "shutting down...");

   exit(1);
}


/*
 * exit with a message
 */

void fatal(char *s){
   printf("%s\n", s);
   cache_down();
}


/*
 * select token data from either cache or database
 */

#ifdef HAVE_MYSQL
   struct token_entry SELECT(MYSQL mysql, struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid, unsigned long ts){
#endif
#ifdef HAVE_SQLITE3
   struct token_entry SELECT(sqlite3 *db, struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid, unsigned long ts){
#endif

   struct qcache *q;
   char stmt[SMALLBUFSIZE];
   struct token_entry res;

   q = findnode(Q, token, uid);
   if(q){
      res.nham = q->nham;
      res.nspam = q->nspam;
      res.hit = 1;
      return res;
   }

   res.nham = res.nspam = res.hit = 0;

#ifdef HAVE_MYSQL
   MYSQL_RES *r;
   MYSQL_ROW row;

   snprintf(stmt, SMALLBUFSIZE-1, "SELECT nham, nspam FROM t_token WHERE token=%llu AND (uid = 0 OR uid=%d)", token, uid);

   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      r = mysql_store_result(&mysql);
      if(r != NULL){
         while((row = mysql_fetch_row(r))){
            res.nham += atol(row[0]);
            res.nspam += atol(row[1]);
         }

         mysql_free_result(r);
      }
   }

#endif
#ifdef HAVE_SQLITE3
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;

   snprintf(stmt, SMALLBUFSIZE-1, "SELECT nham, nspam FROM t_token WHERE token='%llu' AND (uid = 0 OR uid=%d)", token, uid);

   if(sqlite3_prepare_v2(db, stmt, -1, &pStmt, pzTail) != SQLITE_OK) return res;

   while(sqlite3_step(pStmt) == SQLITE_ROW){
      res.nham += sqlite3_column_int(pStmt, 0);
      res.nspam += sqlite3_column_int(pStmt, 1);
   }

   sqlite3_finalize(pStmt);
#endif

   addnode(xhash, token, uid, res.nham, res.nspam, ts);

   return res;
}


/*
 * update a token entry
 */

#ifdef HAVE_MYSQL
   void UPDATE(MYSQL mysql, struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid, unsigned int nham, unsigned int nspam){
#endif
#ifdef HAVE_SQLITE3
   void UPDATE(sqlite3 *db, struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid, unsigned int nham, unsigned int nspam){
#endif

   struct qcache *q;
   char stmt[SMALLBUFSIZE];

   if(nham >= 0 && nspam >= 0){

      q = findnode(Q, token, uid);
      if(q){
         q->nham = nham;
         q->nspam = nspam;
      }

   #ifdef HAVE_MYSQL
      snprintf(stmt, SMALLBUFSIZE-1, "UPDATE %s SET nham=%d, nspam=%d WHERE token=%llu AND uid=%d", SQL_TOKEN_TABLE, nham, nspam, token, uid);
      mysql_real_query(&mysql, stmt, strlen(stmt));
   #endif
   #ifdef HAVE_SQLITE3
      sqlite3_stmt *pStmt;
      const char **pzTail=NULL;

      snprintf(stmt, SMALLBUFSIZE-1, "UPDATE %s SET nham=%d, nspam=%d WHERE token='%llu' AND uid=%d", SQL_TOKEN_TABLE, nham, nspam, token, uid);
      sqlite3_prepare_v2(db, stmt, -1, &pStmt, pzTail);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);

   #endif

   }
   
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
 * process a connection
 */

void *process_connection(void *ptr){
   qconn *QC = (qconn*)ptr;
   struct token_entry res;
   struct timeval tv_start, tv_stop;
   unsigned long long token;
   unsigned long ts;
   unsigned int uid, nham, nspam;
   float total, cache_hit;
   char *p, *q, buf[SMALLBUFSIZE];
   int n;

   total = cache_hit = 0;
   time(&ts);

#ifdef HAVE_MYSQL
   MYSQL mysql;

   mysql_init(&mysql);
   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      send(QC->sockfd, BANNER_QCACHE_450, strlen(BANNER_QCACHE_450), 0);
      goto CLOSE;
   }

#endif
#ifdef HAVE_SQLITE3
   sqlite3 *db;
   int rc;

   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc){
      send(QC->sockfd, BANNER_QCACHE_450, strlen(BANNER_QCACHE_450), 0);
      goto CLOSE;
   }
#endif

   send(QC->sockfd, BANNER_QCACHE_220, strlen(BANNER_QCACHE_220), 0);

   gettimeofday(&tv_start, &tz);

   while((n = recv(QC->sockfd, buf, SMALLBUFSIZE, 0))){

      trim(buf);

      /* determine what command we got */

      p = strchr(buf, ' ');
      if(p){
         *p = '\0';

         /* select command */

         if(strcmp(buf, "SELECT") == 0){
            q = p+1;

            p = strchr(q, ' ');
            if(p){
               *p = '\0';
               token = strtoull(q, NULL, 10);
               uid = atoi(++p);

            #ifdef HAVE_MYSQL
               res = SELECT(mysql, Q, token, uid, ts);
            #endif
            #ifdef HAVE_SQLITE3
               res = SELECT(db, Q, token, uid, ts);
            #endif

               if(res.hit == 1) cache_hit++;
               total++;

               snprintf(buf, SMALLBUFSIZE-1, "250 %d %d\r\n", res.nham, res.nspam);
               send(QC->sockfd, buf, strlen(buf), 0);
            }
            else send(QC->sockfd, QCACHE_RESP_ERR, strlen(QCACHE_RESP_ERR), 0);
         }

         /* update command */

         else if(strcmp(buf, "UPDATE") == 0){
            q = p+1;

            nham = nspam = 0;

            p = strchr(q, ' ');
            if(p){
               *p = '\0';
               token = strtoull(q, NULL, 10);
               q = p+1;
               p = strchr(q, ' ');
               if(p){
                  *p = '\0';
                  uid = atoi(q);
                  q = p+1;
                  p = strchr(q, ' ');
                  if(p){
                     *p = '\0';
                     nham = atoi(q);
                     p++;
                     nspam = atoi(p);

                     if(cfg.qcache_update == 1){
                     #ifdef HAVE_MYSQL
                        UPDATE(mysql, Q, token, uid, nham, nspam);
                     #endif
                     #ifdef HAVE_SQLITE3
                        UPDATE(db, Q, token, uid, nham, nspam);
                     #endif
                     }
                  }
               }
            }
            send(QC->sockfd, QCACHE_RESP_OK, strlen(QCACHE_RESP_OK), 0);


         }
         else send(QC->sockfd, QCACHE_RESP_ERR, strlen(QCACHE_RESP_ERR), 0);
      }
   }

   gettimeofday(&tv_stop, &tz);
   if(total > 0) syslog(LOG_PRIORITY, "cache hit rate: (%.0f of %.0f) %.2f%% in %ld [us]\n", cache_hit, total, 100*cache_hit/total, tvdiff(tv_stop, tv_start));


CLOSE:

#ifdef HAVE_MYSQL
   mysql_close(&mysql);
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(db);
#endif

   close(QC->sockfd);

   decrement_thread_count();
   pthread_exit(0);

   return 0;
}


int main(int argc, char **argv){
   int i, fdmax, newfd, addrlen, yes=1;
   struct sockaddr_in remote_addr;
#ifdef HAVE_TCP
   struct sockaddr_in local_addr;
   struct in_addr addr;
#else
   struct sockaddr_un saun;
#endif
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


   signal(SIGINT, cache_down);
   signal(SIGQUIT, cache_down);
   signal(SIGKILL, cache_down);
   signal(SIGTERM, cache_down);


   cfg = read_config(configfile);


   /* initialize */

   (void) openlog(QCACHE_PROGRAM_NAME, LOG_PID, LOG_MAIL);

   inithash(Q);

   FD_ZERO(&master);
   FD_ZERO(&read_fds);

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   __num_threads=0;


   /* create a listener socket */

#ifdef HAVE_TCP
   if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
#else
   if((listener = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
#endif
      fatal(ERR_OPEN_SOCKET);

#ifdef HAVE_TCP
   local_addr.sin_family = AF_INET;
   local_addr.sin_port = htons(cfg.qcache_port);
   inet_aton(cfg.qcache_addr, &addr);
   local_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(local_addr.sin_zero), 8);
#else
   memset(&saun, 0, sizeof(struct sockaddr_un));
   saun.sun_family = AF_UNIX;
   strcpy(saun.sun_path, QCACHE_SOCKET);

#endif

   if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      fatal(ERR_SET_SOCK_OPT);

#ifdef HAVE_TCP
   if(bind(listener, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1)
#else
   if(bind(listener, (struct sockaddr *)&saun, sizeof(saun.sun_family) + strlen(saun.sun_path) + 1) == -1)
#endif
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
                     send(newfd, BANNER_QCACHE_450, strlen(BANNER_QCACHE_450), 0);
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
