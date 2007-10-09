/*
 * spam-fwd-train.c, 2007.10.05, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <syslog.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "sql.h"
#include "config.h"


#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;
   int rc;
#endif


void my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode);
struct _state parse_message(char *spamfile, struct __config cfg);

#ifdef HAVE_MYSQL
double bayes_file(MYSQL mysql, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
#endif
#ifdef HAVE_SQLITE3
double bayes_file(sqlite3 *db, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
#endif



int main(int argc, char **argv){
   int fd, len, i=0, m, is_spam=0, train_mode=T_TOE;
   char *p, *q, *r, ID[RND_STR_LEN+1]="", *from, buf[8*MAXBUFSIZE], puf[SMALLBUFSIZE];
   double spaminess;
   unsigned long now;
   struct session_data sdata;
   struct __config cfg;
   struct ue UE;
   struct _state state;
   struct _token *P, *Q;
   struct node *tokens[MAXHASH];
   qry QRY;
   time_t clock;

   QRY.uid = 0;

   /* read the default or the given config file */

   if(argc < 2)
      cfg = read_config(CONFIG_FILE);
   else
      cfg = read_config(argv[1]);


   /* make sure training type and (su)rbl won't interfere, 2007.09.29, SJ */

   cfg.training_mode = 0;
   cfg.initial_1000_learning=0;

   cfg.rbl_domain[0] = '\0';
   cfg.surbl_domain[0] = '\0';

   if(strstr(argv[0], "spam-fwd-train")){
      (void) openlog("spam-fwd-train", LOG_PID, LOG_MAIL);
      is_spam = 1;
   }
   else {
      (void) openlog("ham-fwd-train", LOG_PID, LOG_MAIL);
   }

   from = getenv("FROM");
   if(!from){
      syslog(LOG_PRIORITY, "%s", ERR_NO_FROM);
      return 0;
   }

   fd = open("/dev/stdin", O_RDONLY);
   if(fd != -1){
      while((len = read(fd, buf, 8*MAXBUFSIZE)) > 0){
         /* data should be here in the first read */

         p = buf;
         do {
            p = split(p, '\n', puf, SMALLBUFSIZE-1);

            q = strstr(puf, cfg.clapf_header_field);
            if(q){
               r = strchr(puf, ' ');
               if(r){
                  r++;
                  if(is_valid_id(r)){
                     i++;
                     if(i <= 2){
                        snprintf(ID, RND_STR_LEN, "%s", r);
                     }
                  }
               }
            }

            if(strlen(ID) > 2 && strncmp(puf, cfg.clapf_header_field, strlen(cfg.clapf_header_field)) == 0){
               if(strncmp(puf + strlen(cfg.clapf_header_field), "TUM", 3) == 0)
                  train_mode = T_TUM;
            }
         } while(p);
            
      }

      close(fd);
   }

   time(&clock);
   now = clock;

   state = init_state();

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to train", ID);

#ifdef HAVE_MYSQL
   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      syslog(LOG_PRIORITY, "%s", ERR_MYSQL_CONNECT);
      return 0;
   }

   QRY.mysql = mysql;
#endif
#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc){
      syslog(LOG_PRIORITY, "%s", ERR_SQLITE3_OPEN);
      return 0;
   }

   QRY.db = db;
#endif


   /* select uid and username from the user table */

#ifdef HAVE_MYSQL
   UE = get_user_from_email(mysql, from);
#endif
#ifdef HAVE_SQLITE3
   UE = get_user_from_email(db, from);
#endif

   QRY.uid = UE.uid;

   if(!UE.name){
      syslog(LOG_PRIORITY, "not found a username for %s", from);
      goto ENDE;
   }

   snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s", cfg.chrootdir, USER_QUEUE_DIR, UE.name[0], UE.name);

   if(chdir(buf)){
      syslog(LOG_PRIORITY, "cannot chdir() to %s", buf);
      goto ENDE;
   }

   if(cfg.verbosity > 3) syslog(LOG_PRIORITY, "chdir()'ed to: %s", buf);

   if(is_spam == 1)
      snprintf(buf, MAXBUFSIZE-1, "h.%s", ID);
   else
      snprintf(buf, MAXBUFSIZE-1, "s.%s", ID);

   syslog(LOG_PRIORITY, "reading %s", buf);

   state = parse_message(buf, cfg);

   /* select message data */

   make_rnd_string(sdata.ttmpfile);
   sdata.uid = QRY.uid;


   /* if this is a shared group, make sure the token database is trained with uid=0 */

   if(cfg.group_type == GROUP_SHARED)
      QRY.uid = 0;


   if(state.first){

     for(i=0; i<MAX_ITERATIVE_TRAIN_LOOPS; i++){

         inithash(tokens);

         spaminess = DEFAULT_SPAMICITY;
         P = state.first;

         while(P != NULL){
            Q = P->r;
            m = 1;

            addnode(tokens, P->str, 0, 0);
            P = Q;
         }

         QRY.sockfd = -1;

      #ifdef HAVE_QCACHE
         QRY.sockfd = qcache_socket(cfg.qcache_addr, cfg.qcache_port, cfg.qcache_socket);
      #endif

         my_walk_hash(QRY, is_spam, SQL_TOKEN_TABLE, tokens, train_mode);

         if(QRY.sockfd != -1) close(QRY.sockfd);

         if(i == 0){

            if(is_spam == 1){
               if(train_mode == T_TUM)
                  snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1, nham=nham-1 WHERE uid=%ld AND nham > 0", SQL_MISC_TABLE, QRY.uid);
               else
                  snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
            }
            else {
               if(train_mode == T_TUM)
                  snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1, nspam=nspam-1 WHERE uid=%ld AND nspam > 0", SQL_MISC_TABLE, QRY.uid);
               else
                  snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
            }

         #ifdef HAVE_MYSQL   
            mysql_real_query(&mysql, buf, strlen(buf));
         #endif
         #ifdef HAVE_SQLITE3
            sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail);
            sqlite3_step(pStmt);
            sqlite3_finalize(pStmt);
         #endif

            snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (uid, ts, msgid, is_spam) VALUES(%ld, %ld, '%s', %d)", SQL_TRAININGLOG_TABLE, QRY.uid, now, ID, is_spam);

         #ifdef HAVE_MYSQL
            mysql_real_query(&mysql, buf, strlen(buf));
         #endif
         #ifdef HAVE_SQLITE3
            sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail);
            sqlite3_step(pStmt);
            sqlite3_finalize(pStmt);
         #endif


            syslog(LOG_PRIORITY, "%s: training, mode: %d", ID, train_mode);
         }

         clearhash(tokens);

      #ifdef HAVE_MYSQL
         spaminess = bayes_file(mysql, sdata.ttmpfile, state, sdata, cfg);
      #endif
      #ifdef HAVE_SQLITE3
         spaminess = bayes_file(db, sdata.ttmpfile, state, sdata, cfg);
      #endif

         syslog(LOG_PRIORITY, "%s: training round %d, spaminess: %.4f", ID, i, spaminess);

         if(is_spam == 1 && spaminess > cfg.spam_overall_limit) break;
         if(is_spam == 0 && spaminess < cfg.max_ham_spamicity) break;

         /* only the first round can be TUM/TEFT, 2007.09.14, SJ */
         train_mode = T_TOE;
      }

      /* free token list */
      free_and_print_list(state.first, 0);

      unlink(sdata.ttmpfile);
   }


ENDE:

#ifdef HAVE_MYSQL
   mysql_close(&mysql);
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(db);
#endif

   return 1;
}
