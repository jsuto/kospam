/*
 * spam-fwd-train.c, 2007.06.27, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
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
#include "config.h"


void my_walk_hash(MYSQL mysql, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], unsigned int uid, int train_mode);


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
   unsigned long uid=0, cnt, now;
   struct __config cfg;
   struct _state state;
   struct _token *P, *Q;
   struct node *tokens[MAXHASH];
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;

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

   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      syslog(LOG_PRIORITY, "%s", ERR_MYSQL_CONNECT);
      return 0;
   }

   /* select uid */

   snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", cfg.mysqlusertable, from);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "sql: %s", buf);

   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]) uid = atol(row[0]);
         }
         mysql_free_result(res);
      }
   }



   /* select message data */

   snprintf(buf, MAXBUFSIZE-1, "SELECT data FROM %s WHERE id='%s' AND uid=%ld", cfg.mysqlqueuetable, ID, uid);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "sql: %s", buf);

   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]){
               p = row[0];
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


   inithash(tokens);

   if(state.first){

         P = state.first;
         cnt = 0;

         while(P != NULL){
            Q = P->r;
            m = 1;

            addnode(tokens, P->str, 0, 0);

            if(P)
               free(P);

            P = Q;

            cnt++;
         }


         my_walk_hash(mysql, is_spam, cfg.mysqltokentable, tokens, uid, train_mode);


         /* update the t_misc table */

         if(is_spam == 1){
            if(train_mode == T_TUM)
               snprintf(buf, MAXBUFSIZE-1, "update %s set nspam=nspam+1, nham=nham-1 WHERE uid=%ld", cfg.mysqlmisctable, uid);
            else
               snprintf(buf, MAXBUFSIZE-1, "update %s set nspam=nspam+1 WHERE uid=%ld", cfg.mysqlmisctable, uid);
         }
         else {
            if(train_mode == T_TUM)
               snprintf(buf, MAXBUFSIZE-1, "update %s set nham=nham+1, nspam=nspam-1 WHERE uid=%ld", cfg.mysqlmisctable, uid);
            else
               snprintf(buf, MAXBUFSIZE-1, "update %s set nham=nham+1 WHERE uid=%ld", cfg.mysqlmisctable, uid);
         }

         mysql_real_query(&mysql, buf, strlen(buf));

         /* add entry to t_train_log table */

         snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (uid, ts, msgid, is_spam) VALUES(%ld, %ld, '%s', %d)", cfg.mysqltraininglogtable, uid, now, ID, is_spam);
         mysql_real_query(&mysql, buf, strlen(buf));

         syslog(LOG_PRIORITY, "%s: training, mode: %d", ID, train_mode);


   }

   clearhash(tokens);

   mysql_close(&mysql);


   return 1;
}
