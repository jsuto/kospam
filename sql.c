/*
 * sql.c, 2008.01.23, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "score.h"
#include "buffer.h"
#include "sql.h"
#include "config.h"


#ifdef HAVE_MYSQL
   #include <mysql.h>
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
#endif


/*
 * query the spamicity value of a token from token database
 */

float SQL_QUERY(qry QRY, int group_type, char *tokentable, char *token){
   float r = DEFAULT_SPAMICITY;
   struct te TE;

   TE.nham = TE.nspam = 0;

#ifdef HAVE_MYSQL
   TE = myqry(QRY.mysql, QRY.sockfd, tokentable, token, QRY.uid);
#endif
#ifdef HAVE_SQLITE3
   TE = sqlite3_qry(QRY.db, token);
#endif

   if(TE.nham == 0 && TE.nspam == 0) return r;

   r = calc_spamicity(QRY.ham_msg, QRY.spam_msg, TE.nham, TE.nspam, QRY.rob_s, QRY.rob_x);

   return r;
}


/*
 * walk through the hash table and add/update its elements in mysql table
 */

#ifdef HAVE_MYSQL
int my_walk_hash(MYSQL mysql, int sockfd, int ham_or_spam, char *tokentable, unsigned long uid, struct _token *token, int train_mode){
#endif
#ifdef HAVE_SQLITE3
int my_walk_hash(sqlite3 *db, int ham_or_spam, char *tokentable, struct _token *token, int train_mode){
#endif
   int n=0;
   struct _token *p;
   time_t cclock;
   unsigned long now;

   time(&cclock);
   now = cclock;

   p = token;

   while(p != NULL){
   #ifdef HAVE_MYSQL
      do_mysql_qry(mysql, sockfd, ham_or_spam, p->str, tokentable, uid, train_mode);
   #endif
   #ifdef HAVE_SQLITE3
      do_sqlite3_qry(db, ham_or_spam, p->str, train_mode, now);
   #endif
      p = p->r;
      n++;
   }

   return n;
}


/*
 * get uid and username from rcptto
 */

#ifdef HAVE_MYSQL
struct ue get_user_from_email(MYSQL mysql, char *email){
   MYSQL_RES *res;
   MYSQL_ROW row;
#endif
#ifdef HAVE_SQLITE3
struct ue get_user_from_email(sqlite3 *db, char *email){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
#endif
   char *p, buf[MAXBUFSIZE];
   struct ue UE;

   memset((char *)&UE, 0, sizeof(UE));

   if((p = str_case_str(email, "+spam"))){
      *p = '\0';
      snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username FROM %s WHERE email='%s%s'", SQL_USER_TABLE, email, p+5);
   }
   else if((p = str_case_str(email, "+ham"))){
      *p = '\0';
      snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username FROM %s WHERE email='%s%s'", SQL_USER_TABLE, email, p+4);
   }
   else
      snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username FROM %s WHERE email='%s'", SQL_USER_TABLE, email);


#ifdef HAVE_MYSQL
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            UE.uid = atol(row[0]);
            strncpy(UE.name, (char *)row[1], SMALLBUFSIZE-1);
         }               
         mysql_free_result(res);
      }
   }
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         UE.uid = sqlite3_column_int(pStmt, 0);
         strncpy(UE.name, (char *)sqlite3_column_blob(pStmt, 1), SMALLBUFSIZE-1);
      }
   }
   sqlite3_finalize(pStmt);
#endif


   return UE;
}


/*
 * insert metadata to queue table
 */

#ifdef HAVE_MYSQL
int update_training_metadata(MYSQL mysql, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam){
   char *data=NULL;
#endif
#ifdef HAVE_SQLITE3
int update_training_metadata(sqlite3 *db, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
#endif
   struct stat st;
   char buf[MAXBUFSIZE], *map=NULL;
   unsigned long now=0;
   int fd, rc=1;
   time_t clock;

   time(&clock);
   now = clock;

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

   /* then put it into database */

#ifdef HAVE_MYSQL
   data = malloc(2 * st.st_size + strlen(buf) + 1 + 1 + 1);
   if(!data){
      rc = ERR_MALLOC;
      goto ENDE;
   }

   snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (id, uid, is_spam, ts, data) VALUES('%s', %ld, %d, %ld, \"", SQL_QUEUE_TABLE, tmpfile, uid, is_spam, now);
   snprintf(data, 2 * st.st_size + strlen(buf) + 1, "%s", buf);
   mysql_real_escape_string(&mysql, data+strlen(buf), map, st.st_size);
   strncat(data, "\")", 2 * st.st_size + strlen(buf) + 1 + 1);
   mysql_real_query(&mysql, data, strlen(data));

   free(data);
#endif
#ifdef HAVE_SQLITE3
   snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (id, uid, is_spam, ts, data) VALUES('%s', %ld, %d, %ld, ?)", SQL_QUEUE_TABLE, tmpfile, uid, is_spam, now);
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      sqlite3_bind_blob(pStmt, 1, map, st.st_size, SQLITE_STATIC);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);
   }
   else {
      rc = ERR_SQLITE_ERR;
   }
#endif

#ifdef HAVE_MYSQL
ENDE:
#endif

   munmap(map, st.st_size);

   return rc;
}

