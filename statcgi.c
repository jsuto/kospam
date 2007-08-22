/*
 * statcgi.c, 2007.08.22, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "misc.h"
#include "errmsg.h"
#include "messages.h"
#include "cgi.h"
#include "config.h"
#include "cfg.h"

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

FILE *cgiIn, *f, *F;
char *input;

int main(){
   int method=M_UNDEF, timespan=0;
   unsigned long uid=0, nham=0, nspam=0;
   char *p, buf[SMALLBUFSIZE];
   struct tm *t;
   struct __config cfg;
   time_t ts;

   cgiIn = stdin;
   input = NULL;

   printf("Content-type: text/html\n\n");

   cfg = read_config(CONFIG_FILE);

   if(!getenv("REMOTE_USER"))
      errout(NULL, ERR_CGI_NOT_AUTHENTICATED);

#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      errout(NULL, ERR_MYSQL_CONNECT);
#endif
#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc)
      errout(NULL, ERR_SQLITE3_OPEN);
#endif


   if((p = getenv("REQUEST_METHOD"))){

      if(strcmp(p, "POST") == 0)
         method = M_POST;

      else if(strcmp(p, "GET") == 0)
         method = M_GET;
   }

   if(method == M_UNDEF)
      errout(NULL, ERR_CGI_INVALID_METHOD);

   /* determine timespan */

   p = getenv("QUERY_STRING");
   if(p){
      if(strncmp(p, "timespan=", 9) == 0)
         timespan = atoi(p+9);
   }

   printf("<html>\n<title>%s</title>\n<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n<body bgcolor=white text=darkblue vlink=#AC003A>\n<blockquote>\n", CGI_PERSONAL_STAT);

   printf("<h1>%s</h1>\n", CGI_PERSONAL_STAT);

   printf("\n\n<center><a href=\"%s\">%s</a> <a href=\"%s\">%s</a> %s <a href=\"%s\">%s</a></center><p>\n\n\n", cfg.spamcgi_url, CGI_SPAM_QUARANTINE, cfg.usercgi_url, CGI_USER_PREF, CGI_PERSONAL_STAT, cfg.trainlogcgi_url, CGI_TRAIN_LOG);


   /* determine uid in stat table */

   snprintf(buf, SMALLBUFSIZE-1, "SELECT uid FROM %s WHERE username='%s'", SQL_USER_TABLE, getenv("REMOTE_USER"));

#ifdef HAVE_MYSQL
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
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
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW)
         uid = sqlite3_column_int(pStmt, 0);
   }

   sqlite3_finalize(pStmt);
#endif

   if(uid > 0){
      if(timespan == 0)
         printf("<center><strong>%s</strong> <a href=\"%s?timespan=1\">%s</a></center><p/>\n", CGI_DAILY_REPORT, cfg.statcgi_url, CGI_MONTHLY_REPORT);
      else
         printf("<center><a href=\"%s\">%s</a> <strong>%s</strong></center><p/>\n", cfg.statcgi_url, CGI_DAILY_REPORT, CGI_MONTHLY_REPORT);

      printf("<table border=\"1\" align=\"center\">\n");
      printf("<tr align=\"center\"><th>%s</th><th>HAM</th><th>SPAM</th></tr>\n", CGI_DATE);

      if(timespan == 0)
         snprintf(buf, SMALLBUFSIZE-1, "SELECT ts, nham, nspam FROM %s WHERE uid=%ld ORDER BY ts DESC LIMIT 24", SQL_STAT_TABLE, uid);
      else
         snprintf(buf, SMALLBUFSIZE-1, "SELECT FROM_UNIXTIME(ts, '%%Y.%%m.%%d.'), SUM(nham), SUM(nspam) FROM %s WHERE uid=%ld GROUP BY FROM_UNIXTIME(ts, '%%Y.%%m.%%d.') ORDER BY ts DESC", SQL_STAT_TABLE, uid);

   #ifdef HAVE_MYSQL 
      if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
         res = mysql_store_result(&mysql);
         if(res != NULL){
            while((row = mysql_fetch_row(res))){
               nham += atol(row[1]);
               nspam += atol(row[2]);

               if(timespan == 0){
                  ts = atol(row[0]);
                  t = localtime(&ts);
                  printf("<tr align=\"center\"><td>%04d.%02d.%02d %02d:00</td><td>%s</td><td>%s</td></tr>\n", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, row[1], row[2]);
               }
               else
                  printf("<tr align=\"center\"><td>%s</td><td>%s</td><td>%s</td></tr>\n", row[0], row[1], row[2]);                  
            }
            mysql_free_result(res);
            printf("<tr align=\"center\"><td><strong>Total</strong></td><td><strong>%ld</strong></td><td><strong>%ld</strong></td></tr>\n", nham, nspam);
         }
      }
   #endif
   #ifdef HAVE_SQLITE3
      if(sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail) == SQLITE_OK){
         while(sqlite3_step(pStmt) == SQLITE_ROW){
            nham += sqlite3_column_int(pStmt, 1);
            nspam += sqlite3_column_int(pStmt, 2);

            if(timespan == 0){
               ts = sqlite3_column_int(pStmt, 0);
               t = localtime(&ts);
               printf("<tr align=\"center\"><td>%04d.%02d.%02d %02d:00</td><td>%d</td><td>%d</td></tr>\n", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, sqlite3_column_int(pStmt, 1), sqlite3_column_int(pStmt, 2));
            }
            else
               printf("<tr align=\"center\"><td>%ld</td><td>%d</td><td>%d</td></tr>\n", ts, sqlite3_column_int(pStmt, 1), sqlite3_column_int(pStmt, 2));
         }
         printf("<tr align=\"center\"><td><strong>Total</strong></td><td><strong>%ld</strong></td><td><strong>%ld</strong></td></tr>\n", nham, nspam);
      }
      sqlite3_finalize(pStmt);

   #endif

      printf("</table><p>\n");

   }
   else {
      printf("<center>%s</center>\n", ERR_CGI_MYSQL_NO_USER);
   }

#ifdef HAVE_MYSQL
   mysql_close(&mysql);
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(db);
#endif

   printf("</blockquote>\n</body></html>\n");

   return 0;
}
