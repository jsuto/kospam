/*
 * sqlite3.c, SJ
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
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   struct te TE;

   TE.nham = TE.nspam = 0;

   if(sqlite3_prepare_v2(sdata->db, stmt, -1, &pStmt, pzTail) != SQLITE_OK) return TE;

   while(sqlite3_step(pStmt) == SQLITE_ROW){
      TE.nham += sqlite3_column_int(pStmt, 0);
      TE.nspam += sqlite3_column_int(pStmt, 1);
   }

   sqlite3_finalize(pStmt);

   return TE;
}


int update_hash(struct session_data *sdata, char *qry, struct node *xhash[]){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   float nham, nspam;
   unsigned long long token;

   if(sqlite3_prepare_v2(sdata->db, qry, -1, &pStmt, pzTail) != SQLITE_OK) return 0;

   while(sqlite3_step(pStmt) == SQLITE_ROW){
      token = strtoull((char *)sqlite3_column_blob(pStmt, 0), NULL, 10);
      nham = sqlite3_column_double(pStmt, 1);
      nspam = sqlite3_column_double(pStmt, 2);

      updatenode(xhash, token, nham, nspam, DEFAULT_SPAMICITY, 0);
   }

   sqlite3_finalize(pStmt);

   return 1;
}


int introduceTokens(struct session_data *sdata, struct node *xhash[]){
   int i, n=0, ret=0;
   time_t cclock;
   unsigned long now;
   char *err=NULL, s[SMALLBUFSIZE];
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
   if(!query) return ret;

   buffer_cat(query, "BEGIN;");

   time(&cclock);
   now = cclock;

   n = 0;

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(q->nham + q->nspam == 0){
            snprintf(s, SMALLBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, uid, timestamp) VALUES(%llu,0,0,%ld,%ld);", SQL_TOKEN_TABLE, q->key, sdata->gid, now);

            buffer_cat(query, s);
            n++;
         }

         q = q->r;
      }
   }

   buffer_cat(query, "COMMIT;");

   
   if((sqlite3_exec(sdata->db, query->data, NULL, NULL, &err)) == SQLITE_OK) ret = 1;

   buffer_destroy(query);

   return ret;
}


int updateTokenCounters(struct session_data *sdata, int ham_or_spam, struct node *xhash[], int train_mode){
   int i, n=0;
   char *err=NULL, s[SMALLBUFSIZE];
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

   sqlite3_exec(sdata->db, query->data, NULL, NULL, &err);

   buffer_destroy(query);

   return 1;
}


int updateMiscTable(struct session_data *sdata, int ham_or_spam, int train_mode){
   char *err=NULL, s[SMALLBUFSIZE];

   if(ham_or_spam == 1){
      if(train_mode == T_TUM) snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham-1 WHERE uid=%ld AND nham > 0", SQL_MISC_TABLE, sdata->gid);
      else snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->gid);
   } else {
      if(train_mode == T_TUM) snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam-1 WHERE uid=%ld AND nspam > 0", SQL_MISC_TABLE, sdata->gid);
      else snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->gid);
   }

   sqlite3_exec(sdata->db, s, NULL, NULL, &err);

   return 1;
}


int updateTokenTimestamps(struct session_data *sdata, struct node *xhash[]){
   int i, n=0;
   unsigned long now;
   time_t cclock;
   char *err=NULL, s[SMALLBUFSIZE];
   buffer *query;
   struct node *q;

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


   if((sqlite3_exec(sdata->db, query->data, NULL, NULL, &err)) != SQLITE_OK)
      n = -1;

   buffer_destroy(query);

   return n;
}

