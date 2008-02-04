/*
 * sqlite3.c, 2008.01.23, SJ
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


/*
 * update the token timestamps
 */

int update_sqlite3_tokens(sqlite3 *db, struct _token *token){
   int n=0;
   unsigned long now;
   time_t cclock;
   char *err=NULL, buf[SMALLBUFSIZE];
   buffer *query;
   struct _token *p, *q;

   if(token == NULL) return 0;

   query = buffer_create(NULL);
   if(!query) return n;

   time(&cclock);
   now = cclock;

   snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET timestamp=%ld WHERE token in (", SQL_TOKEN_TABLE, now);

   buffer_cat(query, buf);

   p = token;

   while(p != NULL){
      q = p->r;

      snprintf(buf, SMALLBUFSIZE-1, "%llu, ", APHash(p->str));
      buffer_cat(query, buf);

      p = q;
   }

   snprintf(buf, SMALLBUFSIZE-1, "0)");
   buffer_cat(query, buf);

   if((sqlite3_exec(db, query->data, NULL, NULL, &err)) != SQLITE_OK)
      n = -1;

   buffer_destroy(query);

   return n;
}



/*
 * updates the counter of (or inserts) the given token in the token table
 */

int do_sqlite3_qry(sqlite3 *db, int ham_or_spam, char *token, int train_mode, unsigned long timestamp){
   char stmt[MAXBUFSIZE], puf[SMALLBUFSIZE];
   struct te TE;
   unsigned long long hash = APHash(token);
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;

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

      sqlite3_prepare_v2(db, stmt, -1, &pStmt, ppzTail);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);

   }

   /* ... or insert token entry */

   else {
      if(ham_or_spam == 1)
         TE.nspam = 1;
      else
         TE.nham = 1;

      snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, timestamp) VALUES(%llu, %d, %d, %ld)", SQL_TOKEN_TABLE, hash, TE.nham, TE.nspam, timestamp);

      sqlite3_prepare_v2(db, stmt, -1, &pStmt, ppzTail);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);

   }

   return 0;
}


/*
 * insert email entry to queue table
 */

void insert_2_queue(sqlite3 *db, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam){
   char buf[SMALLBUFSIZE], *err=NULL;
   unsigned long now=0;
   time_t clock;

   time(&clock);
   now = clock;

   snprintf(buf, SMALLBUFSIZE-1, "INSERT INTO %s (id, uid, is_spam, ts) VALUES('%s', %ld, %d, %ld)", SQL_QUEUE_TABLE, tmpfile, uid, is_spam, now);

   sqlite3_exec(db, buf, NULL, NULL, &err);
}

