/*
 * mysql.c, 2007.07.05, SJ
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

struct te get_ham_spam(MYSQL mysql, char *stmt){
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

int do_mysql_qry(MYSQL mysql, int sockfd, int ham_or_spam, char *token, char *tokentable, unsigned int uid, int train_mode){
   char stmt[MAXBUFSIZE], puf[SMALLBUFSIZE];
   struct te TE;
   unsigned long long hash = APHash(token);

#ifdef HAVE_NO_64_HASH
   char buf[MAXBUFSIZE];
   if(strlen(token) < (MAXBUFSIZE/2)-1){
      mysql_real_escape_string(&mysql, buf, token, strlen(token));
      snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token='%s' AND uid=%d", tokentable, buf, uid);
   }
   else
      return 1;
#else
   snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND uid=%d", tokentable, hash, uid);
#endif

   /* update token entry ... */

#ifdef HAVE_QCACHE
   memset(puf, 0, SMALLBUFSIZE);
   TE = myqry(mysql, sockfd, tokentable, token, uid);

   if(ham_or_spam == 1){
      if(train_mode == T_TUM && TE.nham > 0) TE.nham--;
      snprintf(stmt, MAXBUFSIZE-1, "UPDATE %llu %d %d %d\r\n", hash, uid, TE.nham, TE.nspam+1);
   }
   else {
      if(train_mode == T_TUM && TE.nspam > 0) TE.nspam--;
      snprintf(stmt, MAXBUFSIZE-1, "UPDATE %llu %d %d %d\r\n", hash, uid, TE.nham+1, TE.nspam);
   }

   if(sockfd != -1){
      send(sockfd, stmt, strlen(stmt), 0);
      recv(sockfd, stmt, MAXBUFSIZE, 0);
   }
#else
   TE = get_ham_spam(mysql, stmt);

   if(TE.nham > 0 || TE.nspam > 0){
      if(ham_or_spam == 1){
         if(train_mode == T_TUM && TE.nham > 0) snprintf(puf, SMALLBUFSIZE-1, ", nham=nham-1");
      #ifdef HAVE_NO_64_HASH
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1%s WHERE token='%s' AND uid=%d", tokentable, puf, buf, uid);
      #else
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1%s WHERE token=%llu AND uid=%d", tokentable, puf, hash, uid);
      #endif
      }
      else {
         if(train_mode == T_TUM && TE.nspam > 0) snprintf(puf, SMALLBUFSIZE-1, ", nspam=nspam-1");
      #ifdef HAVE_NO_64_HASH
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1%s WHERE token='%s' AND uid=%d", tokentable, puf, buf, uid);
      #else
         snprintf(stmt, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1%s WHERE token=%llu AND uid=%d", tokentable, puf, hash, uid);
      #endif
      }

      mysql_real_query(&mysql, stmt, strlen(stmt));
   }
#endif


   /* ... or insert token entry */

   if(TE.nham == 0 && TE.nspam == 0){
      if(ham_or_spam == 1)
         TE.nspam = 1;
      else
         TE.nham = 1;

   #ifdef HAVE_NO_64_HASH
      if(uid > 0)
         snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, uid) VALUES('%s', %ld, %ld, %d)", tokentable, buf, TE.nham, TE.nspam, uid);
      else
         snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam) VALUES('%s', %ld, %ld)", tokentable, buf, TE.nham, TE.nspam);
   #else
      if(uid > 0)
         snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, uid) VALUES(%llu, %d, %d, %d)", tokentable, hash, TE.nham, TE.nspam, uid);
      else
         snprintf(stmt, MAXBUFSIZE-1, "INSERT INTO %s (token, nham, nspam) VALUES(%llu, %d, %d)", tokentable, hash, TE.nham, TE.nspam);
   #endif

      mysql_real_query(&mysql, stmt, strlen(stmt));
   }


   return 0;
}



/*
 * query the number of occurances from MySQL table
 */

struct te myqry(MYSQL mysql, int sockfd, char *tokentable, char *token, unsigned int uid){
   struct te TE;
   char stmt[MAXBUFSIZE];

   TE.nham = 0;
   TE.nspam = 0;

#ifdef HAVE_NO_64_HASH
   char buf[MAXBUFSIZE];
   if(strlen(token) < (MAXBUFSIZE/2)-1){
      mysql_real_escape_string(&mysql, buf, token, strlen(token));
      snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token='%s' AND (uid=0 OR uid=%d)", tokentable, buf, uid);
   }
   else
      return TE;
#else
   unsigned long long hash = APHash(token);
   snprintf(stmt, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND (uid=0 OR uid=%d)", tokentable, hash, uid);
#endif

#ifdef HAVE_QCACHE
   char *p, *q;

   snprintf(stmt, MAXBUFSIZE-1, "SELECT %llu %d\r\n", hash, uid);
   send(sockfd, stmt, strlen(stmt), 0);
   memset(stmt, 0, MAXBUFSIZE);
   recv(sockfd, stmt, MAXBUFSIZE, 0);

   /* is it a valid response (status code: 250) */

   if(strncmp(stmt, "250 ", 4) == 0){
      p = stmt+4;
      q = strchr(p, ' ');
      if(q){
        *q = '\0';
        TE.nham = atof(p);
        TE.nspam = atof(++q);
      }
   }

#else

   MYSQL_RES *res;
   MYSQL_ROW row;

   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            TE.nham += atol(row[0]);
            TE.nspam += atol(row[1]);
         }

         mysql_free_result(res);
      }
   }
#endif

   return TE;
}


/*
 * insert metadata to queue table
 */

int update_training_metadata(MYSQL mysql, char *tmpfile, char rcptto[MAX_RCPT_TO][MAXBUFSIZE], int num_of_rcpt_to, struct __config cfg){
   struct stat st;
   MYSQL_RES *res;
   MYSQL_ROW row;
   char *p, *q, buf[MAXBUFSIZE], email[SMALLBUFSIZE], *map=NULL, *data=NULL;
   unsigned long now=0, uid;
   int i, fd;
   time_t clock;

   time(&clock);
   now = clock;

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

         snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", SQL_USER_TABLE, p);
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

                     snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (id, uid, ts, data) VALUES('%s', %ld, %ld, \"", SQL_QUEUE_TABLE, tmpfile, uid, now);

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


   return 1;
}


