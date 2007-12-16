/*
 * sqlite3.c, 2007.12.16, SJ
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
#include "config.h"


/*
 * query the number of occurances from SQLite3 table
 */

struct te sqlite3_qry(sqlite3 *db, int sockfd, char *tokentable, char *token, unsigned int uid){
   struct te TE;
   char stmt[MAXBUFSIZE];
   unsigned long long hash = APHash(token);

   TE.nham = TE.nspam = 0;

#ifdef HAVE_QCACHE
   char *p, *q;

   snprintf(stmt, MAXBUFSIZE-1, "SELECT %llu %d\r\n", hash, uid);
   send(sockfd, stmt, strlen(stmt), 0);
   memset(stmt, 0, MAXBUFSIZE);
   recv(sockfd, stmt, MAXBUFSIZE, 0);

   /* is it a valid response (status code: 250) */

   if(strncmp(stmt, "250 ", 4) == 0){
      p = strchr(stmt, ' ');
      if(p){
         *p = '\0';
         q = strchr(++p, ' ');
         if(q){
            *q = '\0';
            TE.nham = atof(p);
            TE.nspam = atof(++q);
         }
      }
   }

#else
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;

   snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token='%llu' AND (uid=0 OR uid=%d)", tokentable, hash, uid);

   if(sqlite3_prepare_v2(db, stmt, -1, &pStmt, pzTail) != SQLITE_OK) return TE;

   while(sqlite3_step(pStmt) == SQLITE_ROW){
      TE.nham += sqlite3_column_int(pStmt, 0);
      TE.nspam += sqlite3_column_int(pStmt, 1);
   }

   sqlite3_finalize(pStmt);
#endif

   return TE;
}


/*
 * updates the counter of (or inserts) the given token in the token table
 */

int do_sqlite3_qry(sqlite3 *db, int sockfd, int ham_or_spam, char *token, char *tokentable, unsigned int uid, int train_mode){
   char stmt[MAXBUFSIZE], puf[SMALLBUFSIZE];
   struct te TE;
   unsigned long long hash = APHash(token);
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;

   memset(puf, 0, SMALLBUFSIZE);

   /* update token entry ... */

   TE = sqlite3_qry(db, sockfd, tokentable, token, uid);

   if(TE.nham > 0 || TE.nspam > 0){
      if(ham_or_spam == 1){
         if(train_mode == T_TUM && TE.nham > 0) snprintf(puf, SMALLBUFSIZE-1, ", nham=nham-1");
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1%s WHERE token='%llu' AND uid=%d", tokentable, puf, hash, uid);
      }
      else {
         if(train_mode == T_TUM && TE.nspam > 0) snprintf(puf, SMALLBUFSIZE-1, ", nspam=nspam-1");
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1%s WHERE token='%llu' AND uid=%d", tokentable, puf, hash, uid);
      }

      sqlite3_prepare_v2(db, stmt, -1, &pStmt, ppzTail);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);

   }



   /* ... or insert token entry */

   if(TE.nham == 0 && TE.nspam == 0){
      if(ham_or_spam == 1)
         TE.nspam = 1;
      else
         TE.nham = 1;

      snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, uid) VALUES('%llu', %d, %d, %d)", tokentable, hash, TE.nham, TE.nspam, uid);

      sqlite3_prepare_v2(db, stmt, -1, &pStmt, ppzTail);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);

   }

   return 0;
}

