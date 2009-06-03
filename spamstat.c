/*
 * spamstat.c, 2009.06.03, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include "misc.h"
#include "cfg.h"
#include "messages.h"
#include "config.h"


#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;
   int rc;
#endif


int main(int argc, char **argv){
   char *p, buf[MAXBUFSIZE], puf[MAXBUFSIZE];
   unsigned long uid, now, nham, nspam;
   struct __config cfg;
   FILE *f;
   time_t clock;

   if(argc < 2)
      cfg = read_config(CONFIG_FILE);
   else
      cfg = read_config(argv[1]);

   time(&clock);
   now = clock;

#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      __fatal(ERR_MYSQL_CONNECT);
#endif
#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc)
      __fatal(ERR_SQLITE3_OPEN);
#endif
 
   f = fopen("/dev/stdin", "r");
   if(!f)
      __fatal(ERR_CANNOT_OPEN);


   while(fgets(buf, MAXBUFSIZE-1, f)){
      //email@address 5 2

      uid = nham = nspam = 0;

      p = strrchr(buf, ' ');
      if(p){
         *p = '\0';
         nspam = atol(p+1);
         p = strrchr(buf, ' ');

         if(p){
            *p = '\0';
             nham = atol(p+1);
         }
      }

      if(nham + nspam > 0){

         /* get uid if we have to */

         if(!strchr(buf, '@')){
            snprintf(puf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", SQL_EMAIL_TABLE, buf);

         #ifdef HAVE_MYSQL

            if(mysql_real_query(&mysql, puf, strlen(puf)) == 0){
               res = mysql_store_result(&mysql);
               if(res != NULL){
                  row = mysql_fetch_row(res);
                  if(row)
                     uid = atol(row[0]);
                  mysql_free_result(res);
               }
            }

         #endif
         #ifdef HAVE_SQLITE3

            if(sqlite3_prepare_v2(db, puf, -1, &pStmt, ppzTail) == SQLITE_OK){
               if(sqlite3_step(pStmt) == SQLITE_ROW)
                  uid = sqlite3_column_int(pStmt, 0);
            }

            sqlite3_finalize(pStmt);
         #endif
         }
         else {
            uid = atoi(buf);
         }


         snprintf(puf, MAXBUFSIZE-1, "INSERT INTO %s (uid, ts, nham, nspam) VALUES(%ld, %ld, %ld, %ld)", SQL_STAT_TABLE, uid, now, nham, nspam);

         #ifdef HAVE_MYSQL
            mysql_real_query(&mysql, puf, strlen(puf));
         #endif
         #ifdef HAVE_SQLITE3
            sqlite3_prepare_v2(db, puf, -1, &pStmt, ppzTail);
            sqlite3_step(pStmt);
            sqlite3_finalize(pStmt);
         #endif

      }
   }

   fclose(f);

#ifdef HAVE_MYSQL
   mysql_close(&mysql);
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(db);
#endif

   return 0;
}

