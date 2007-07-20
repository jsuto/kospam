/*
 * spam-fwd-train.c, 2007.07.11, SJ
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
#include <mysql.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "sql.h"
#include "config.h"

void my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode);
double bayes_file(MYSQL mysql, char *spamfile, struct session_data sdata, struct __config cfg);

/*
 * check if it's a valid ID
 */

int is_valid_id(char *p){

   if(strlen(p) != 30)
      return 0;

   for(; *p; p++){
      /* 0-9: 0x30-0x39, a-f: 0x61-0x66 */

      if(! ((*p >= 0x30 && *p <= 0x39) || (*p >= 0x61 && *p <= 0x66) ) ){
         printf("%c*\n", *p);
         return 0;
      }
   }

   return 1;
}


int main(int argc, char **argv){
   int fd, len, i=0, m, is_spam=0, train_mode=T_TOE;
   char *p, *q, *r, ID[RND_STR_LEN+1]="", *from, buf[8*MAXBUFSIZE], puf[SMALLBUFSIZE];
   double spaminess;
   unsigned long now;
   struct session_data sdata;
   struct __config cfg;
   struct _state state;
   struct _token *P, *Q;
   struct node *tokens[MAXHASH];
   qry QRY;
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;
   FILE *f;

   QRY.uid = 0;

   /* read the default or the given config file */

   if(argc < 2)
      cfg = read_config(CONFIG_FILE);
   else
      cfg = read_config(argv[1]);

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
                     if(i == 2){
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

   time(&now);

   state = init_state();

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to train", ID);

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      syslog(LOG_PRIORITY, "%s", ERR_MYSQL_CONNECT);
      return 0;
   }

   QRY.mysql = mysql;
#endif


   /* select uid */

   snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", SQL_USER_TABLE, from);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "sql: %s", buf);

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]) QRY.uid = atol(row[0]);
         }
         mysql_free_result(res);
      }
   }
#endif



   /* select message data */

   make_rnd_string(sdata.ttmpfile);
   sdata.uid = QRY.uid;

   snprintf(buf, MAXBUFSIZE-1, "SELECT data FROM %s WHERE id='%s' AND uid=%ld", SQL_QUEUE_TABLE, ID, QRY.uid);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "sql: %s", buf);

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]){
               p = row[0];

               f = fopen(sdata.ttmpfile, "w+");
               if(f){
                  fprintf(f, "%s", p);
                  fclose(f);
               }

               do {
                  p = split(p, '\n', buf, MAXBUFSIZE-2);
                  strncat(buf, "\n", MAXBUFSIZE-1);
                  state = parse(buf, state);
               } while(p);
            }
         }
         mysql_free_result(res);
      }
   }
#endif


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
                  snprintf(buf, MAXBUFSIZE-1, "update %s set nspam=nspam+1, nham=nham-1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
               else
                  snprintf(buf, MAXBUFSIZE-1, "update %s set nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
            }
            else {
               if(train_mode == T_TUM)
                  snprintf(buf, MAXBUFSIZE-1, "update %s set nham=nham+1, nspam=nspam-1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
               else
                  snprintf(buf, MAXBUFSIZE-1, "update %s set nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
            }

         #ifdef HAVE_MYSQL_TOKEN_DATABASE   
            mysql_real_query(&mysql, buf, strlen(buf));
         #endif

            snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (uid, ts, msgid, is_spam) VALUES(%ld, %ld, '%s', %d)", SQL_TRAININGLOG_TABLE, QRY.uid, now, ID, is_spam);

         #ifdef HAVE_MYSQL_TOKEN_DATABASE
            mysql_real_query(&mysql, buf, strlen(buf));
         #endif


            syslog(LOG_PRIORITY, "%s: training, mode: %d", ID, train_mode);
         }

         clearhash(tokens);

         spaminess = bayes_file(mysql, sdata.ttmpfile, sdata, cfg);

         syslog(LOG_PRIORITY, "%s: training round %d, spaminess: %.4f", ID, i, spaminess);

         if(is_spam == 1 && spaminess > cfg.spam_overall_limit) break;
         if(is_spam == 0 && spaminess < cfg.max_junk_spamicity) break;

      }

      /* free token list */
      free_and_print_list(state.first, 0);

      unlink(sdata.ttmpfile);
   }


   mysql_close(&mysql);


   return 1;
}
