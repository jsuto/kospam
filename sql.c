/*
 * sql.c, 2009.01.22, SJ
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
 * walk through the hash table and add/update its elements in sql table
 */

#ifdef HAVE_MYSQL
int my_walk_hash(MYSQL mysql, int ham_or_spam, unsigned long uid, struct node *xhash[], int train_mode){
#endif
#ifdef HAVE_SQLITE3
int my_walk_hash(sqlite3 *db, int ham_or_spam, struct node *xhash[], int train_mode){
#endif
   int i, n=0;
   time_t cclock;
   unsigned long now;
   struct node *q;

   time(&cclock);
   now = cclock;

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
      #ifdef HAVE_MYSQL
         do_mysql_qry(mysql, ham_or_spam, q->str, uid, train_mode, now);
      #endif
      #ifdef HAVE_SQLITE3
         do_sqlite3_qry(db, ham_or_spam, q->str, train_mode, now);
      #endif
         
         q = q->r;
         n++;
      }
   }

   return n;
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

