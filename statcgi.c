/*
 * statcgi.c, 2007.05.18, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <unistd.h>
#include <time.h>
#include "misc.h"
#include "errmsg.h"
#include "messages.h"
#include "cgi.h"
#include "config.h"
#include "cfg.h"

FILE *cgiIn, *f, *F;
char *input;
MYSQL mysql;
MYSQL_RES *res;
MYSQL_ROW row;

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

   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      errout(NULL, ERR_MYSQL_CONNECT);


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

   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row)
            uid = atol(row[0]);

         mysql_free_result(res);
      }
   }

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
      printf("</table><p>\n");

   }
   else {
      printf("<center>%s</center>\n", ERR_CGI_MYSQL_NO_USER);
   }

   mysql_close(&mysql);

   printf("</blockquote>\n</body></html>\n");

   return 0;
}
