/*
 * mysql.c, 2010.05.13, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
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
#include "sql.h"
#include "buffer.h"
#include "score.h"
#include "defs.h"
#include "config.h"


/*
 * get uid of the user included in the sql query
 */

unsigned long get_uid(MYSQL mysql, char *stmt){
   unsigned long uid = 0;
   MYSQL_RES *res;
   MYSQL_ROW row;

   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      if((res = mysql_store_result(&mysql))){
         row = mysql_fetch_row(res);
         if(row)
            uid = atol(row[0]);

         mysql_free_result(res);
      }
   }

   return uid;
}


/*
 * get the total number of ham and spam
 */

struct te getNumberOfHamSpamMessages(MYSQL mysql, char *stmt){
   struct te TE;
   MYSQL_RES *res;
   MYSQL_ROW row;

   TE.nham = TE.nspam = 0;

   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            TE.nham += atof(row[0]);
            TE.nspam += atof(row[1]);
         }
         mysql_free_result(res);
      }
   }
   return TE;
}


/*
 * updates the counter of (or inserts) the given token in the token table
 */

int do_mysql_qry(MYSQL mysql, int ham_or_spam, char *token, unsigned long uid, int train_mode, unsigned long timestamp){
   char stmt[MAXBUFSIZE], puf[SMALLBUFSIZE];
   struct te TE;
   unsigned long long hash = APHash(token);

   snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND uid=%ld", SQL_TOKEN_TABLE, hash, uid);

   memset(puf, 0, SMALLBUFSIZE);

   /* update token entry ... */

   TE = getNumberOfHamSpamMessages(mysql, stmt);

   if(TE.nham > 0 || TE.nspam > 0){
      if(ham_or_spam == 1){
         if(train_mode == T_TUM && TE.nham > 0) snprintf(puf, SMALLBUFSIZE-1, ", nham=nham-1");
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1%s WHERE token=%llu AND uid=%ld", SQL_TOKEN_TABLE, puf, hash, uid);
      }
      else {
         if(train_mode == T_TUM && TE.nspam > 0) snprintf(puf, SMALLBUFSIZE-1, ", nspam=nspam-1");
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1%s WHERE token=%llu AND uid=%ld", SQL_TOKEN_TABLE, puf, hash, uid);
      }
   }

   /* ... or insert token entry */

   if(TE.nham == 0 && TE.nspam == 0){
      if(ham_or_spam == 1)
         TE.nspam = 1;
      else
         TE.nham = 1;

      if(uid > 0)
         snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, uid, timestamp) VALUES(%llu, %d, %d, %ld, %ld)", SQL_TOKEN_TABLE, hash, TE.nham, TE.nspam, uid, timestamp);
      else
         snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, timestamp) VALUES(%llu, %d, %d, %ld)", SQL_TOKEN_TABLE, hash, TE.nham, TE.nspam, timestamp);
   }

   mysql_real_query(&mysql, stmt, strlen(stmt));

   return 0;
}



int update_hash(MYSQL mysql, char *qry, struct node *xhash[], struct __config *cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   float nham, nspam;
   unsigned long long token;

   if(mysql_real_query(&mysql, qry, strlen(qry)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            token = strtoull(row[0], NULL, 10);
            nham = atof(row[1]);
            nspam = atof(row[2]);

            updatenode(xhash, token, nham, nspam, DEFAULT_SPAMICITY, 0);
         }

         mysql_free_result(res);
      }

   }

   return 1;
}


/*
 * update the token timestamps
 */

int update_mysql_tokens(struct session_data *sdata, struct node *xhash[]){
   int i, n=0;
   unsigned long now;
   time_t cclock;
   char s[SMALLBUFSIZE];
   struct node *p, *q;
   buffer *query;

   if(counthash(xhash) <= 0) return 0;

   query = buffer_create(NULL);
   if(!query) return n;

   time(&cclock);
   now = cclock;

   snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET timestamp=%ld WHERE token in (0", SQL_TOKEN_TABLE, now);

   buffer_cat(query, s);

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

         if(p->spaminess != DEFAULT_SPAMICITY){
            snprintf(s, SMALLBUFSIZE-1, ",%llu", p->key);
            buffer_cat(query, s);
            n++;
         }

         q = q->r;
      }
   }


/* disabled this feature for now, 2010.02.16, SJ */

#ifdef HAVE_MEMCACHED_DISABLED
   int memcached_ok=0;
   char *qry, memcached_queue_id[BUFLEN];
   uint32_t flags = sdata->uid;
   uint64_t qid;

   struct timezone tz;
   struct timeval tv1, tv2;

   gettimeofday(&tv1, &tz);

   if(sdata->memc != NULL){

      if(memcached_increment(sdata->memc, MEMCACHED_KEY_NAME, MEMCACHED_KEY_LENGTH, 1, &qid) != MEMCACHED_SUCCESS){

         if(memcached_add(sdata->memc, MEMCACHED_KEY_NAME, MEMCACHED_KEY_LENGTH, "1", 1, 0, flags) == MEMCACHED_SUCCESS){
            memcached_ok = 1;
            qid = 1;
         }
      }
      else {
         memcached_ok = 1;
      }

      if(memcached_ok == 1){
         snprintf(memcached_queue_id, BUFLEN-1, "%s%llu", MEMCACHED_MESSAGE_NAME, qid);

         qry = strstr(query->data, "(0,");
         if(qry && strlen(qry) > 8){
            if(memcached_add(sdata->memc, memcached_queue_id, strlen(memcached_queue_id), qry+3, strlen(qry)-3, 0, flags) != MEMCACHED_SUCCESS){
               memcached_ok = 0;
            }
         }
         else {
            memcached_ok = 1;
         }
      }
   }

   gettimeofday(&tv2, &tz);
   //syslog(LOG_PRIORITY, "%s: memcached exec time: %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

   if(memcached_ok == 0){
      syslog(LOG_PRIORITY, "%s: exec()ing sql update", sdata->ttmpfile);
#endif

      if(sdata->uid > 0)
         snprintf(s, SMALLBUFSIZE-1, "0) AND (uid=0 OR uid=%ld)", sdata->uid);
      else
         snprintf(s, SMALLBUFSIZE-1, "0) AND uid=0");

      buffer_cat(query, s);

      if(mysql_real_query(&(sdata->mysql), query->data, strlen(query->data)) != 0){
         n = -1;
      }
#ifdef HAVE_MEMCACHED_DISABLED
   }
#endif

   buffer_destroy(query);

   return n;
}


/*
 * walk through the hash table and add/update its elements in sql table
 */

int my_walk_hash(MYSQL mysql, int ham_or_spam, unsigned long uid, struct node *xhash[], int train_mode){
   int i, n=0;
   time_t cclock;
   unsigned long now;
   struct node *q;

   time(&cclock);
   now = cclock;

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         do_mysql_qry(mysql, ham_or_spam, q->str, uid, train_mode, now);
         
         q = q->r;
         n++;
      }
   }

   return n;
}


