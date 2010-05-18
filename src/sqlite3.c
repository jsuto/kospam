/*
 * sqlite3.c, 2009.02.17, SJ
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
#include <sqlite3.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "sql.h"
#include "score.h"
#include "buffer.h"
#include "config.h"


/*
 * query the number of occurances from SQLite3 table
 */

struct te sqlite3_qry(sqlite3 *db, char *token){
   struct te TE;
   char stmt[MAXBUFSIZE];
   unsigned long long hash = APHash(token);
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;

   TE.nham = TE.nspam = 0;

   snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu", SQL_TOKEN_TABLE, hash);

   if(sqlite3_prepare_v2(db, stmt, -1, &pStmt, pzTail) != SQLITE_OK) return TE;

   while(sqlite3_step(pStmt) == SQLITE_ROW){
      TE.nham += sqlite3_column_int(pStmt, 0);
      TE.nspam += sqlite3_column_int(pStmt, 1);
   }

   sqlite3_finalize(pStmt);

   return TE;
}


int update_hash(sqlite3 *db, char *qry, struct node *xhash[], struct __config *cfg){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   float nham, nspam;
   unsigned long long token;

   if(sqlite3_prepare_v2(db, qry, -1, &pStmt, pzTail) != SQLITE_OK) return 0;

   while(sqlite3_step(pStmt) == SQLITE_ROW){
      token = strtoull((char *)sqlite3_column_blob(pStmt, 0), NULL, 10);
      nham = sqlite3_column_double(pStmt, 1);
      nspam = sqlite3_column_double(pStmt, 2);

      updatenode(xhash, token, nham, nspam, DEFAULT_SPAMICITY, 0);
   }

   sqlite3_finalize(pStmt);

   return 1;
}


/*
 * update the token timestamps
 */

int update_sqlite3_tokens(struct session_data *sdata, struct node *xhash[]){
   int i, n=0;
   unsigned long now;
   time_t cclock;
   char *err=NULL, buf[SMALLBUFSIZE];
   buffer *query;
   struct node *q;

   if(counthash(xhash) <= 0) return 0;

   query = buffer_create(NULL);
   if(!query) return n;

   time(&cclock);
   now = cclock;

   snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET timestamp=%ld WHERE token in (0", SQL_TOKEN_TABLE, now);

   buffer_cat(query, buf);


   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(q->spaminess != DEFAULT_SPAMICITY){
            snprintf(buf, SMALLBUFSIZE-1, ",%llu", q->key);
            buffer_cat(query, buf);
            n++;
         }

         q = q->r;
      }
   }



   snprintf(buf, SMALLBUFSIZE-1, "0)");
   buffer_cat(query, buf);

   if((sqlite3_exec(sdata->db, query->data, NULL, NULL, &err)) != SQLITE_OK)
      n = -1;

   buffer_destroy(query);

   return n;
}



/*
 * updates the counter of (or inserts) the given token in the token table
 */

int do_sqlite3_qry(sqlite3 *db, int ham_or_spam, char *token, int train_mode, unsigned long timestamp){
   char stmt[MAXBUFSIZE], puf[SMALLBUFSIZE], *err=NULL;
   struct te TE;
   unsigned long long hash = APHash(token);

   memset(puf, 0, SMALLBUFSIZE);

   TE = sqlite3_qry(db, token);

   /* update token entry ... */

   if(TE.nham + TE.nspam > 0){
      if(ham_or_spam == 1){
         if(train_mode == T_TUM && TE.nham > 0) snprintf(puf, SMALLBUFSIZE-1, ", nham=nham-1");
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1%s WHERE token=%llu", SQL_TOKEN_TABLE, puf, hash);
      }
      else {
         if(train_mode == T_TUM && TE.nspam > 0) snprintf(puf, SMALLBUFSIZE-1, ", nspam=nspam-1");
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1%s WHERE token=%llu", SQL_TOKEN_TABLE, puf, hash);
      }
   }

   /* ... or insert token entry */

   else {
      if(ham_or_spam == 1)
         TE.nspam = 1;
      else
         TE.nham = 1;

      snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, timestamp) VALUES(%llu, %d, %d, %ld)", SQL_TOKEN_TABLE, hash, TE.nham, TE.nspam, timestamp);
   }

   sqlite3_exec(db, stmt, NULL, NULL, &err);
   
   return 0;
}


/*
 * walk through the hash table and add/update its elements in sql table
 */

int my_walk_hash(sqlite3 *db, int ham_or_spam, struct node *xhash[], int train_mode){
   int i, n=0;
   time_t cclock;
   unsigned long now;
   struct node *q;

   time(&cclock);
   now = cclock;

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         do_sqlite3_qry(db, ham_or_spam, q->str, train_mode, now);
         
         q = q->r;
         n++;
      }
   }

   return n;
}

