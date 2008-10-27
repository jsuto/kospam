/*
 * clapfadmincgi.c, 2008.10.20, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "cgi.h"
#include "errmsg.h"
#include "messages.h"

#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL_RES *res;
   MYSQL_ROW row;
   MYSQL mysql;
#endif

FILE *cgiIn;
struct __config cfg;
int admin_user = 0;


unsigned long next_uid(){
   unsigned long uid=0;
   char stmt[MAXBUFSIZE];

   snprintf(stmt, MAXBUFSIZE-1, "SELECT MAX(uid)+1 FROM %s", SQL_USER_TABLE);

#ifdef HAVE_MYSQL
   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&mysql);
      if(res){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]) uid = atol(row[0]);
         }
         mysql_free_result(res);
      }
   }
#endif

   if(uid == 0) uid = 1;

   return uid;
}


int main(){
   char *p, buf[MAXBUFSIZE], user[SMALLBUFSIZE], email[SMALLBUFSIZE], admin_menu[SMALLBUFSIZE];
   int clen=0, method=M_UNDEF, n=0, uid=0;
   struct cgidata cgi;
   char *input=NULL;

   cgiIn = stdin;
   memset(admin_menu, 0, SMALLBUFSIZE);

   memset(user, 0, SMALLBUFSIZE);
   memset(email, 0, SMALLBUFSIZE);

   cfg = read_config(CONFIG_FILE);

   printf("Content-type: text/html\n\n");

   /* check request method */

   if((p = getenv("REQUEST_METHOD"))){
      if(strcmp(p, "GET") == 0)
         method = M_GET;

      if(strcmp(p, "POST") == 0)
         method = M_POST;
   }

   p = getenv("REMOTE_USER");
   if(!p)
      errout(input, ERR_CGI_NOT_AUTHENTICATED);


   /* if you are an administrator */
   if(strcmp(p, cfg.admin_user) == 0){
      admin_user = 1;
   }


   printf("<html>\n<title>%s</title>\n<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n<body bgcolor=white text=darkblue vlink=#AC003A>\n<blockquote>\n", CGI_USER_MANAGEMENT);

   printf("<h1>%s</h1>\n", CGI_USER_MANAGEMENT);

   show_cgi_menu(cfg, admin_user, CGI_USER_MANAGEMENT);

   if(admin_user == 0){
      printf("%s\n</body></html>\n", ERR_CGI_YOU_NOT_ADMIN);
      return 0;
   } 


   if(method == M_GET)
      cgi = extract_cgi_parameters(getenv("QUERY_STRING"));
   else if(method == M_POST){
      if((p = getenv("CONTENT_LENGTH")))
         clen = atoi(p);

      input = (char *) malloc(clen);
      if(!input)
         errout(input, ERR_CGI_NO_MEMORY);

      memset(input, 0, clen);

      if(((int) fread(input, 1, clen, stdin)) != clen)
         errout(input, ERR_CGI_POST_READ);

      input[clen] = '\0';

      cgi = extract_cgi_parameters(input);
   }


#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0) == 0){
      printf("%s\n</body></html>\n", ERR_MYSQL_CONNECT);
      return 0;
   }

   if(strlen(cgi.user) < (SMALLBUFSIZE/2)-1)
      mysql_real_escape_string(&mysql, user, cgi.user, strlen(cgi.user));

   if(strlen(cgi.email) < (SMALLBUFSIZE/2)-1){
      url_decode(cgi.email);
      mysql_real_escape_string(&mysql, email, cgi.email, strlen(cgi.email));
   }

#endif

 
   if(method == M_GET){

      /* remove the given user from the user table */

      if(strncmp(getenv("QUERY_STRING"), "remove=1&", 9) == 0){

      #ifdef HAVE_MYSQL
         snprintf(buf, SMALLBUFSIZE-1, "DELETE FROM %s WHERE username='%s'", SQL_USER_TABLE, user);

         if(mysql_real_query(&mysql, buf, strlen(buf)))
            printf("%s", ERR_CGI_CANNOT_REMOVE);
         else
            printf("%s", ERR_CGI_USER_SUCCESSFULLY_REMOVED);
      #endif

         printf(". <a href=\"%s\">%s</a>", cfg.clapfadmincgi_url, ERR_CGI_BACK);
      }

      /* show edit form for the given user */

      else if(strncmp(getenv("QUERY_STRING"), "edit=1&", 7) == 0){
         printf("<form action=\"%s\" name=\"edituser\" method=\"post\">\n", cfg.clapfadmincgi_url);
         printf("<table border=\"0\">\n");

         snprintf(buf, SMALLBUFSIZE-1, "SELECT email, uid FROM %s WHERE username='%s'", SQL_USER_TABLE, user);
      #ifdef HAVE_MYSQL
         if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
            res = mysql_store_result(&mysql);
            if(res != NULL){
               row = mysql_fetch_row(res);
               if(row){
                  printf("<input type=\"hidden\" name=\"edit\" value=\"1\">\n");
                  printf("<input type=\"hidden\" name=\"user\" value=\"%s\">\n", cgi.user);
                  printf("<tr><td>%s:</td><td><input type=\"text\" name=\"email\" value=\"%s\"></td></tr>\n", CGI_EMAIL, row[0]);
                  printf("<tr><td>%s:</td><td><input type=\"text\" name=\"userid\" value=\"%s\"></td></tr>\n", CGI_USERID, row[1]);
               }
               mysql_free_result(res);
            }
         }
      #endif

         printf("<tr colspan=\"2\"><td><input type=\"submit\" value=\"OK\"></td></tr>\n");
         printf("</table>\n");
         printf("</form>\n");
      }

      else {
         /* add new user form */

         printf("%s<p/>\n", CGI_ADD_NEW_USER);

         printf("<form action=\"%s\" name=\"adduser\" method=\"post\">\n", cfg.clapfadmincgi_url);
         printf("<input type=\"hidden\" name=\"add\" value=\"1\">\n");
         printf("<table border=\"0\">\n");
         printf("<tr><td>%s:</td><td><input type=\"text\" name=\"email\" ></td></tr>\n", CGI_EMAIL);
         printf("<tr><td>%s:</td><td><input type=\"text\" name=\"user\" ></td></tr>\n", CGI_USER);
         printf("<tr><td>%s:</td><td><input type=\"text\" name=\"userid\" ></td></tr>\n", CGI_USERID);
         printf("<tr colspan=\"2\"><td><input type=\"submit\" value=\"OK\"></td></tr>\n");
         printf("</table>\n");
         printf("</form>\n");



         /* list current users */

         printf("<p/><table border=\"0\">\n<tr align=\"middle\"><th>%s</th><th>%s</th><th>%s</th><th>&nbsp;</th></tr>\n", CGI_EMAIL, CGI_USER, CGI_USERID);

         snprintf(buf, SMALLBUFSIZE-1, "SELECT email, username, uid FROM %s", SQL_USER_TABLE);

         n = 0;

      #ifdef HAVE_MYSQL
         if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
            res = mysql_store_result(&mysql);
            if(res != NULL){
               while((row = mysql_fetch_row(res))){
                  n++;

                  if(n % 2)
                     printf("<tr class=\"odd\">");
                  else
                     printf("<tr>");

                  printf("<td><a href=\"%s?edit=1&user=%s\">%s</a></td><td>%s</td><td>%s</td><td><a href=\"%s?remove=1&user=%s\">%s</a></td></tr>\n\n", cfg.clapfadmincgi_url, row[1], row[0], row[1], row[2], cfg.clapfadmincgi_url, row[1], ERR_CGI_REMOVE);
               }
               mysql_free_result(res);
            }
         }
      #endif

         printf("</table><p/>\n");
      }

   #ifdef HAVE_MYSQL
      mysql_close(&mysql);
   #endif
   }

   /* add a new user */

   if(method == M_POST){
      if(strlen(cgi.email) < 4 || strlen(cgi.user) < 1) errout(input, ERR_MISSING_DATA);

      /* sanity check variables */

      uid = 0;

      if(strlen(cgi.userid) >= 1) uid = atoi(cgi.userid);
      else uid = next_uid();

      if(uid < 0) uid = 0;

      //printf("i: %s * email: %s, uid: %s", input, cgi.email, cgi.userid);

   #ifdef HAVE_MYSQL
      if(strncmp(input, "add=1", 5) == 0){
         snprintf(buf, SMALLBUFSIZE-1, "INSERT INTO %s (email, username, uid) VALUES('%s', '%s', '%d')", SQL_USER_TABLE, email, user, uid);

         if(mysql_real_query(&mysql, buf, strlen(buf)))
            printf("%s", ERR_CGI_EXISTING_USER);
         else
            printf("%s", ERR_CGI_NEW_USER_SUCCESSFULLY_ADDED);
      }
      else if(strncmp(input, "edit=1", 6) == 0){
         snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET email='%s', uid=%d WHERE username='%s'", SQL_USER_TABLE, email, uid, user);

         if(mysql_real_query(&mysql, buf, strlen(buf)))
            printf("%s", ERR_CGI_CANNOT_MODIFY);
         else
            printf("%s", ERR_CGI_NEW_USER_SUCCESSFULLY_MODIFIED);
      }

      mysql_close(&mysql);
   #endif

      printf(". <a href=\"%s\">%s</a>", cfg.clapfadmincgi_url, ERR_CGI_BACK);
   }


   return 0;
}
