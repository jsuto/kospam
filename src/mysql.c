/*
 * mysql.c, SJ
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
#include <sys/mman.h>
#include <clapf.h>


struct te getHamSpamCounters(struct session_data *sdata, char *stmt){
   struct te TE;
   MYSQL_RES *res;
   MYSQL_ROW row;

   TE.nham = TE.nspam = 0;

   if(mysql_real_query(&(sdata->mysql), stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&(sdata->mysql));
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


int update_hash(struct session_data *sdata, char *qry, struct node *xhash[]){
   MYSQL_RES *res;
   MYSQL_ROW row;
   float nham, nspam;
   unsigned long long token;

   if(mysql_real_query(&(sdata->mysql), qry, strlen(qry)) == 0){
      res = mysql_store_result(&(sdata->mysql));
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
 * introduce tokens to database with zero (0) counter values
 */

int introduceTokens(struct session_data *sdata, struct node *xhash[]){
   int i, n=0;
   time_t cclock;
   unsigned long now;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   if(counthash(xhash) <= 0) return 0;

   query = buffer_create(NULL);
   if(!query) return 0;

   snprintf(s, SMALLBUFSIZE-1, "SELECT token, nham, nspam FROM %s WHERE token in (", SQL_TOKEN_TABLE);
   buffer_cat(query, s);

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(n) snprintf(s, SMALLBUFSIZE-1, ",%llu", q->key);
         else snprintf(s, SMALLBUFSIZE-1, "%llu", q->key);

         buffer_cat(query, s);
         n++;

         q = q->r;
      }
   }

   snprintf(s, SMALLBUFSIZE-1, ") AND uid=%ld", sdata->gid);
   buffer_cat(query, s);

   update_hash(sdata, query->data, xhash);

   buffer_destroy(query);


   query = buffer_create(NULL);
   if(!query) return 0;

   snprintf(s, SMALLBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, uid, timestamp) VALUES", SQL_TOKEN_TABLE);
   buffer_cat(query, s);

   time(&cclock);
   now = cclock;

   n = 0;

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(q->nham + q->nspam == 0){
            if(n) snprintf(s, SMALLBUFSIZE-1, ",(%llu,0,0,%ld,%ld)", q->key, sdata->gid, now);
            else snprintf(s, SMALLBUFSIZE-1, "(%llu,0,0,%ld,%ld)", q->key, sdata->gid, now);

            buffer_cat(query, s);
            n++;
         }

         q = q->r;
      }
   }

   mysql_real_query(&(sdata->mysql), query->data, strlen(query->data));

   buffer_destroy(query);

   return 1;
}


int updateTokenCounters(struct session_data *sdata, int ham_or_spam, struct node *xhash[], int train_mode){
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   if(counthash(xhash) <= 0) return 0;

   query = buffer_create(NULL);
   if(!query) return n;

   if(ham_or_spam == 1){
      if(train_mode == T_TUM) snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham-1 WHERE token IN (", SQL_TOKEN_TABLE);
      else snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam+1 WHERE token IN (", SQL_TOKEN_TABLE); 
   } else {
      if(train_mode == T_TUM) snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam-1 WHERE token IN (", SQL_TOKEN_TABLE); 
      else snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham+1 WHERE token IN (", SQL_TOKEN_TABLE);
   }

   buffer_cat(query, s);

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(n) snprintf(s, SMALLBUFSIZE-1, ",%llu", q->key);
         else snprintf(s, SMALLBUFSIZE-1, "%llu", q->key);

         buffer_cat(query, s);

         q = q->r;
         n++;
      }
   }

   buffer_cat(query, ")");

   if(train_mode == T_TUM){
      if(ham_or_spam == 1) buffer_cat(query, " AND nham > 0");
      else buffer_cat(query, " AND nspam > 0");
   }

   snprintf(s, SMALLBUFSIZE-1, " AND uid=%ld", sdata->gid);
   buffer_cat(query, s);

   mysql_real_query(&(sdata->mysql), query->data, strlen(query->data));

   buffer_destroy(query);

   return 1;
}


int updateMiscTable(struct session_data *sdata, int ham_or_spam, int train_mode){
   char s[SMALLBUFSIZE];

   if(ham_or_spam == 1){
      if(train_mode == T_TUM) snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham-1 WHERE uid=%ld AND nham > 0", SQL_MISC_TABLE, sdata->gid);
      else snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->gid);
   } else {
      if(train_mode == T_TUM) snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam-1 WHERE uid=%ld AND nspam > 0", SQL_MISC_TABLE, sdata->gid);
      else snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->gid);
   }

   mysql_real_query(&(sdata->mysql), s, strlen(s));

   return 1;
}


int updateTokenTimestamps(struct session_data *sdata, struct node *xhash[]){
   int i, n=0;
   unsigned long now;
   time_t cclock;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   if(counthash(xhash) <= 0) return 0;

   query = buffer_create(NULL);
   if(!query) return n;

   time(&cclock);
   now = cclock;

   snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET timestamp=%ld WHERE token in (", SQL_TOKEN_TABLE, now);

   buffer_cat(query, s);

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(q->spaminess != DEFAULT_SPAMICITY){
            if(n) snprintf(s, SMALLBUFSIZE-1, ",%llu", q->key);
            else snprintf(s, SMALLBUFSIZE-1, "%llu", q->key);
            buffer_cat(query, s);
            n++;
         }

         q = q->r;
      }
   }


   if(sdata->gid > 0)
      snprintf(s, SMALLBUFSIZE-1, ") AND (uid=0 OR uid=%ld)", sdata->gid);
   else
      snprintf(s, SMALLBUFSIZE-1, ") AND uid=0");

   buffer_cat(query, s);

   if(mysql_real_query(&(sdata->mysql), query->data, strlen(query->data)) != 0){
      n = -1;
   }

   buffer_destroy(query);

   return n;
}

