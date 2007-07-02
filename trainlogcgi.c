/*
 * trainlog.c, 2007.05.18, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <time.h>
#include <unistd.h>
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
   int is_spam, method=M_UNDEF;
   unsigned long ts, uid=0;
   char *p, buf[SMALLBUFSIZE];
   struct tm *t;
   struct __config cfg;

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


   printf("<html>\n<title>%s</title>\n<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n<body bgcolor=white text=darkblue vlink=#AC003A>\n<blockquote>\n", CGI_TRAIN_LOG);

   printf("<h1>%s</h1>\n", CGI_TRAIN_LOG);

   printf("\n\n<center><a href=\"%s\">%s</a> <a href=\"%s\">%s</a> <a href=\"%s\">%s</a> %s</center><p>\n\n\n", cfg.spamcgi_url, CGI_SPAM_QUARANTINE, cfg.usercgi_url, CGI_USER_PREF, cfg.statcgi_url, CGI_PERSONAL_STAT, CGI_TRAIN_LOG);


   /* determine uid in stat table */

   snprintf(buf, SMALLBUFSIZE-1, "SELECT uid FROM %s WHERE username='%s'", cfg.mysqlusertable, getenv("REMOTE_USER"));

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

      printf("<table border=\"1\" align=\"center\">\n");
      printf("<tr align=\"center\"><th>%s</th><th>%s</th><th>%s</th></tr>\n", CGI_DATE, CGI_MESSAGE, CGI_HAM_OR_SPAM);

      snprintf(buf, SMALLBUFSIZE-1, "SELECT ts, msgid, is_spam FROM %s WHERE uid=%ld ORDER BY ts DESC", cfg.mysqltraininglogtable, uid);

      if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
         res = mysql_store_result(&mysql);
         if(res != NULL){
            while((row = mysql_fetch_row(res))){
               ts = atol(row[0]);
               t = localtime(&ts);

               is_spam = atoi(row[2]);

               printf("<tr align=\"center\"><td>%04d.%02d.%02d %02d:%02d:%02d</td><td>%s</td>", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, row[1]);
               if(is_spam == 0)
                  printf("<td>ham</td></tr>\n");
               else
                  printf("<td>spam</td></tr>\n");

            }
            mysql_free_result(res);
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
