/*
 * mysql.c, 2007.06.26, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <mysql.h>
#include <sys/mman.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "config.h"


/*
 * updates the counter of (or inserts) the given token in the token table
 */

int do_mysql_qry(MYSQL mysql, int ham_or_spam, char *token, char *tokentable, unsigned int uid, int train_mode){
   MYSQL_RES *res;
   MYSQL_ROW row;
   unsigned long nham=0, nspam=0;
   char stmt[MAXBUFSIZE], puf[SMALLBUFSIZE];

   memset(puf, 0, SMALLBUFSIZE);

#ifdef HAVE_NO_64_HASH
   char buf[MAXBUFSIZE];
   if(strlen(token) < (MAXBUFSIZE/2)-1){
      mysql_real_escape_string(&mysql, buf, token, strlen(token));
      snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token='%s' AND uid=%d", tokentable, buf, uid);
   }
   else
      return 1;
#else
   unsigned long long hash = APHash(token);
   snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND uid=%d", tokentable, hash, uid);
#endif

   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]) nham = atol(row[0]);
            if(row[1]) nspam = atol(row[1]);

            /* update statement */

            if(ham_or_spam == 1){
               if(train_mode == T_TUM && nham > 0) snprintf(puf, SMALLBUFSIZE-1, ", nham=nham-1");

               #ifdef HAVE_NO_64_HASH
                  snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1%s WHERE token='%s' AND uid=%d", tokentable, puf, buf, uid);
               #else
                  snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1%s WHERE token=%llu AND uid=%d", tokentable, puf, hash, uid);
               #endif
            }
            else {
               if(train_mode == T_TUM && nspam > 0) snprintf(puf, SMALLBUFSIZE-1, ", nspam=nspam-1");

               #ifdef HAVE_NO_64_HASH
                  snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1%s WHERE token='%s' AND uid=%d", tokentable, puf, buf, uid);
               #else
                  snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1%s WHERE token=%llu AND uid=%d", tokentable, puf, hash, uid);
               #endif
            }

         }
         else {
            /* insert statement */

            if(ham_or_spam == 1)
               nspam = 1;
            else
               nham = 1;

         #ifdef HAVE_NO_64_HASH
            if(uid > 0)
               snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, uid) VALUES('%s', %ld, %ld, %d)", tokentable, buf, nham, nspam, uid);
            else
               snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam) VALUES('%s', %ld, %ld)", tokentable, buf, nham, nspam);
         #else
            if(uid > 0)
               snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, uid) VALUES(%llu, %ld, %ld, %d)", tokentable, hash, nham, nspam, uid);
            else
               snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam) VALUES(%llu, %ld, %ld)", tokentable, hash, nham, nspam);
         #endif
         }

         mysql_free_result(res);

         /* now teach the token database */
         mysql_real_query(&mysql, stmt, strlen(stmt));
      }
   }

   return 0;
}


/*
 * walk through the hash table and add/update its elements in mysql table
 */

int my_walk_hash(MYSQL mysql, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], unsigned int uid, int train_mode){
   int i, n=0;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

         do_mysql_qry(mysql, ham_or_spam, p->str, tokentable, uid, train_mode);
         n++;

         q = q->r;
         if(p)
            free(p);
      }
      xhash[i] = NULL;
   }

   return n;
}



/*
 * query the spamicity value of a token from token database
 */

float myqry(MYSQL mysql, char *tokentable, char *token, float ham_msg, float spam_msg, unsigned int uid, struct node *xhash[MAXHASH]){
   MYSQL_RES *res;
   MYSQL_ROW row;
   float nham=0, nspam=0;
   float r = DEFAULT_SPAMICITY, ham_prob, spam_prob;
   char stmt[MAXBUFSIZE];
   int n;

#ifdef HAVE_NO_64_HASH
   char buf[MAXBUFSIZE];
   if(strlen(token) < (MAXBUFSIZE/2)-1){
      mysql_real_escape_string(&mysql, buf, token, strlen(token));
      snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token='%s' AND (uid=0 OR uid=%d)", tokentable, buf, uid);
   }
   else
      return r;
#else
   unsigned long long hash = APHash(token);
   snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND (uid=0 OR uid=%d)", tokentable, hash, uid);
#endif

   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            nham += atol(row[0]);
            nspam += atol(row[1]);
         }

         /* if the token has occurred at least N times, 2007.06.13, SJ */

         if(nham + nspam > 1){
            ham_prob = nham / ham_msg;
            spam_prob = nspam / spam_msg;

            if(ham_prob > 1) ham_prob = 1;
            if(spam_prob > 1) spam_prob = 1;

            r = spam_prob / (ham_prob + spam_prob);

            /* deal with rare words */

            if(nham < FREQ_MIN && nspam < FREQ_MIN){
               n = nham;
               if(nspam > n) n = nspam;

               r = (0.5 + n * r) / (1+n);
            }

         }

         mysql_free_result(res);

         /* add token to list if not mature enough, 2007.06.13, SJ */
         if(uid > 0 && nham < TUM_LIMIT && nspam < TUM_LIMIT)
            addnode(xhash, token, 0 , 0);

      }
   }

   if(r < REAL_HAM_TOKEN_PROBABILITY) r = REAL_HAM_TOKEN_PROBABILITY;
   if(r > REAL_SPAM_TOKEN_PROBABILITY) r = REAL_SPAM_TOKEN_PROBABILITY;

   return r;
}


/*
 * insert metadata to queue table
 */

int update_training_metadata(char *tmpfile, char rcptto[MAX_RCPT_TO][MAXBUFSIZE], int num_of_rcpt_to, struct __config cfg){
   struct stat st;
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;
   char *p, *q, buf[MAXBUFSIZE], email[SMALLBUFSIZE], *map=NULL, *data=NULL;
   unsigned long now=0, uid;
   int i, fd;

   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      syslog(LOG_PRIORITY, "%s: %s", tmpfile, ERR_MYSQL_CONNECT);
      return 0;
   }

   time(&now);

   for(i=0; i<num_of_rcpt_to; i++){
      p = q = NULL;

      snprintf(email, SMALLBUFSIZE-1, "%s", rcptto[i]);
      p = strchr(email, '<');
      if(p){
         q = strchr(p, '>');
      }

      if(p && q){
         *q = '\0';

         uid=0;
         p++;

         /* fix address like spam+aaa@domain.com */
         q = strchr(p, '+');
         if(q) p = q+1;

         snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", cfg.mysqlusertable, p);
         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s sql: %s", tmpfile, buf);

         if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
            res = mysql_store_result(&mysql);
            if(res != NULL){
               row = mysql_fetch_row(res);
               if(row){
                  uid = atol(row[0]);

                  if(uid > 0){

                     /* reading message file into memory, 2007.06.26, SJ */

                     if(stat(tmpfile, &st)){
                        syslog(LOG_PRIORITY, "cannot stat: %s", tmpfile);
                        return ERR_STAT_SPAM_FILE;
                     }

                     fd = open(tmpfile, O_RDONLY);
                     if(fd == -1)
                        return ERR_BAYES_OPEN_SPAM_FILE;

                     map = mmap(map, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
                     close(fd);
                     if(map == NULL)
                        return ERR_BAYES_MMAP;

                     snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (id, uid, ts, data) VALUES('%s', %ld, %ld, \"", cfg.mysqlqueuetable, tmpfile, uid, now);

                     data = malloc(2 * st.st_size + strlen(buf) + 1 + 1 + 1);
                     if(data != NULL){
                        snprintf(data, 2 * st.st_size + strlen(buf) + 1, "%s", buf);
                        mysql_real_escape_string(&mysql, data+strlen(buf), map, st.st_size);
                        strncat(data, "\")", 2 * st.st_size + strlen(buf) + 1 + 1);
                        mysql_real_query(&mysql, data, strlen(data));

                        free(data);
                     }
                     munmap(map, st.st_size);
                  }
               }

               mysql_free_result(res);
            }
         }
      }
   }

   mysql_close(&mysql);

   return 1;
}


