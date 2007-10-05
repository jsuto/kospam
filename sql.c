/*
 * sql.c, 2007.10.05, SJ
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
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "sql.h"
#include "config.h"


#ifdef HAVE_MYSQL
   #include <mysql.h>
   int do_mysql_qry(MYSQL mysql, int sockfd, int ham_or_spam, char *token, char *tokentable, unsigned int uid, int train_mode);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   int do_sqlite3_qry(sqlite3 *db, int sockfd, int ham_or_spam, char *token, char *tokentable, unsigned int uid, int train_mode);
#endif

/*
 * query the spamicity value of a token from token database
 */

float SQL_QUERY(qry QRY, char *tokentable, char *token, struct node *xhash[MAXHASH]){
   float r = DEFAULT_SPAMICITY, ham_prob=0, spam_prob=0;
   int n;
   struct te TE;

   TE.nham = TE.nspam = 0;

#ifdef HAVE_MYSQL
   TE = myqry(QRY.mysql, QRY.sockfd, tokentable, token, QRY.uid);
#endif
#ifdef HAVE_SQLITE3
   TE = sqlite3_qry(QRY.db, QRY.sockfd, tokentable, token, QRY.uid);
#endif

   /* add token to list if not mature enough, 2007.07.09, SJ */

   if(QRY.uid > 0 && TE.nham < TUM_LIMIT && TE.nspam < TUM_LIMIT)
      addnode(xhash, token, 0 , 0);

   if(TE.nham == 0 && TE.nspam == 0) return r;


   /* if the token has occurred at least N=2 times, 2007.06.13, SJ */

   if(TE.nham + TE.nspam > 2){
      if(QRY.ham_msg > 0) ham_prob = TE.nham / QRY.ham_msg;
      else                ham_prob = 0;

      if(QRY.spam_msg > 0) spam_prob = TE.nspam / QRY.spam_msg;
      else                 spam_prob = 0;

      if(ham_prob > 1) ham_prob = 1;
      if(spam_prob > 1) spam_prob = 1;

      if(ham_prob + spam_prob > 0) r = spam_prob / (ham_prob + spam_prob);

      /* deal with rare words */

      if(TE.nham < FREQ_MIN && TE.nspam < FREQ_MIN){
         n = TE.nham;
         if(TE.nspam > n) n = TE.nspam;

         r = (QRY.rob_s * QRY.rob_x + n * r) / (QRY.rob_s + n);
      }

   }

   if(r < REAL_HAM_TOKEN_PROBABILITY) r = REAL_HAM_TOKEN_PROBABILITY;
   if(r > REAL_SPAM_TOKEN_PROBABILITY) r = REAL_SPAM_TOKEN_PROBABILITY;

   return r;
}


/*
 * walk through the hash table and add/update its elements in mysql table
 */

int my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode){
   int i, n=0;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

      #ifdef HAVE_MYSQL
         do_mysql_qry(QRY.mysql, QRY.sockfd, ham_or_spam, p->str, tokentable, QRY.uid, train_mode);
      #endif
      #ifdef HAVE_SQLITE3
         do_sqlite3_qry(QRY.db, QRY.sockfd, ham_or_spam, p->str, tokentable, QRY.uid, train_mode);
      #endif

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
 * get uid and username from rcptto
 */

#ifdef HAVE_MYSQL
struct ue get_user_from_email(MYSQL mysql, char *rcptto){
   MYSQL_RES *res;
   MYSQL_ROW row;
#endif
#ifdef HAVE_SQLITE3
struct ue get_user_from_email(sqlite3 *db, char *rcptto){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
#endif
   char *p, *q, buf[MAXBUFSIZE], email[SMALLBUFSIZE];
   struct ue UE;

   memset((char *)&UE, 0, sizeof(UE));

   snprintf(email, SMALLBUFSIZE-1, "%s", rcptto);
   p = strchr(email, '<');
   if(p){
      q = strchr(p, '>');
      if(q) *q = '\0';
      p++;
   }
   else
      p = email;

   if(p){
      /* fix address like spam+aaa@domain.com */

      q = strchr(p, '+');
      if(q) p = q+1;

      snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username FROM %s WHERE email='%s'", SQL_USER_TABLE, p);

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

   }

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

