/*
 * spamcgi.c, 2008.05.14, SJ
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

FILE *cgiIn;
struct __config cfg;
int admin_user = 0;


char *extract(char *row, int ch, char *s, int size);
int deliver_message(char *dir, char *message, char *username, struct __config cfg);


int main(){
   char *p, *q, m[SMALLBUFSIZE], msg[SMALLBUFSIZE], spamqdir[MAXBUFSIZE], user[SMALLBUFSIZE], admin_menu[SMALLBUFSIZE], mailfrom[SMALLBUFSIZE], subject[SMALLBUFSIZE];
   int clen=0, method=M_UNDEF, n=0, n_spam=0;
   struct cgidata cgi;
   char *input=NULL;

   cgiIn = stdin;
   memset(admin_menu, 0, SMALLBUFSIZE);

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
      snprintf(admin_menu, SMALLBUFSIZE-1, "<a href=\"%s\">%s</a> ", cfg.spamcgi_url, CGI_USER_LIST);
   }


   printf("<html>\n<title>%s</title>\n<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n<body bgcolor=white text=darkblue vlink=#AC003A>\n<blockquote>\n", CGI_SPAM_QUARANTINE);

   printf("<h1>%s</h1>\n", CGI_SPAM_QUARANTINE);

   printf("\n\n<center>%s%s <a href=\"%s\">%s</a> <a href=\"%s\">%s</a> <a href=\"%s\">%s</a></center>\n\n\n", admin_menu, CGI_SPAM_QUARANTINE, cfg.usercgi_url, CGI_USER_PREF, cfg.statcgi_url, CGI_PERSONAL_STAT, cfg.trainlogcgi_url, CGI_TRAIN_LOG);

   printf("<script type=\"text/javascript\">\n\nfunction mark_all(x){\n   var i;\n   var len = document.forms[0].elements.length;\n\n   for(i=0; i<len; i++)\n      document.forms[0].elements[i].checked = x;\n}\n\n</script>\n\n");




   if(method == M_GET){
      cgi = extract_cgi_parameters(getenv("QUERY_STRING"));

      /* fix username */
      if(admin_user == 1)
         snprintf(user, SMALLBUFSIZE-1, "%s", cgi.user);
      else
         snprintf(user, SMALLBUFSIZE-1, "%s", getenv("REMOTE_USER"));

      snprintf(spamqdir, MAXBUFSIZE-1, "%s/%c/%s", USER_QUEUE_DIR, user[0], user);

      snprintf(mailfrom, SMALLBUFSIZE-1, "%s", cgi.from);
      snprintf(subject, SMALLBUFSIZE-1, "%s", cgi.subject);


      /* show selected message ... */

      if(strlen(cgi.id) > 1){

      #ifdef HAVE_USERDB
         printf("<a href=\"%s?delivery=%s&user=%s\">%s</a><br>\n", cfg.spamcgi_url, cgi.id, user, ERR_CGI_DELIVER_AND_REMOVE);
      #else
         printf("<a href=\"%s?delivery=%s&user=%s\">%s</a><br>\n", cfg.spamcgi_url, cgi.id, user, ERR_CGI_REMOVE);
      #endif


         printf("<a href=\"%s?train=%s&user=%s\">%s</a><br>\n", cfg.traincgi_url, cgi.id, user, ERR_CGI_DELIVER_AND_TRAIN_AS_HAM);

         printf("<br>\n\n<pre>\n");
         show_message(spamqdir, cgi.id);
         printf("</pre>\n");
      }

      /* or deliver message */

      else if(strlen(cgi.delivery) > 1){

      #ifdef HAVE_USERDB
         if(deliver_message(spamqdir, cgi.delivery, user, cfg) == OK){

            snprintf(m, SMALLBUFSIZE-1, "%s/%s", spamqdir, cgi.delivery);
            if(unlink(m) == 0) printf("%s (%s).<p>\n<a href=\"%s?user=%s\">Back.</a>\n", ERR_CGI_REMOVED, cgi.delivery, cfg.spamcgi_url, user);
            else printf("%s (%s).<p>\n<a href=\"%s?user=%s\">Back.</a>\n", ERR_CGI_FAILED_TO_REMOVE, cgi.delivery, cfg.spamcgi_url, user);
         }
         else {
            printf("%s (%s)\n", ERR_CGI_DELIVERY_FAILED, cgi.delivery);
         }
      #else
         snprintf(m, SMALLBUFSIZE-1, "%s/%s", spamqdir, cgi.delivery);
         if(unlink(m) == 0) printf("%s (%s).<p>\n<a href=\"%s?user=%s\">Back.</a>\n", ERR_CGI_REMOVED, cgi.delivery, cfg.spamcgi_url, user);
         else printf("%s (%s).<p>\n<a href=\"%s?user=%s\">Back.</a>\n", ERR_CGI_FAILED_TO_REMOVE, cgi.delivery, cfg.spamcgi_url, user);
      #endif

      }

      /* ... or scan directory */

      else {

         if(admin_user == 1 && strlen(cgi.user) < 1)
            show_users(USER_QUEUE_DIR, cfg.spamcgi_url);
         else {
            url_decode(cgi.from);
            url_decode(cgi.subject);

            printf("<form action=\"%s\" name=\"aaa0\" method=\"gett\">\n", cfg.spamcgi_url);
            //printf("<input type=\"hidden\" name=\"page\" value=\"%d\">\n", cgi.page);
            printf("<input type=\"hidden\" name=\"user\" value=\"%s\">\n", cgi.user);
            printf("<table border=\"0\">\n");
            printf("<tr><td>%s:</td><td><input type=\"text\" name=\"from\" value=\"%s\"></td></tr>\n", CGI_FROM, cgi.from);
            printf("<tr><td>%s:</td><td><input type=\"text\" name=\"subject\" value=\"%s\"></td></tr>\n", CGI_SUBJECT, cgi.subject);
            printf("<tr colspan=\"2\"><td><input type=\"submit\" value=\"OK\"></td></tr>\n");
            printf("</table>\n");
            printf("</form>\n\n");

            printf("<form action=\"%s\" name=\"aaa1\" method=\"post\">\n", cfg.spamcgi_url);
            printf("<input type=\"hidden\" name=\"topurge\" value=\"1\">\n");
            printf("<input type=\"hidden\" name=\"user\" value=\"%s\">\n", user);

            n_spam = check_directory(spamqdir, user, cgi.page, cfg.page_len, cfg.spamcgi_url, cgi.from, cgi.subject);

            printf("<input type=\"submit\" value=\"%s\"> <input type=\"reset\" value=\"%s\">\n<input type=\"button\" value=\"%s\" onClick=\"mark_all(true)\"></form><p>\n", ERR_CGI_PURGE_SELECTED, ERR_CGI_CANCEL, ERR_CGI_SELECT_ALL);
         }

         if(cgi.page > 0){
            printf("<a href=\"%s?page=0&user=%s&from=%s&subject=%s\">First</a> <a href=\"%s?page=%d&user=%s&from=%s&subject=%s\">Previous</a>\n", cfg.spamcgi_url, user, mailfrom, subject, cfg.spamcgi_url, cgi.page-1, user, mailfrom, subject);
         }

         if(n_spam >= cfg.page_len*(cgi.page+1) && n_spam > cfg.page_len)
            printf(" <a href=\"%s?page=%d&user=%s&from=%s&subject=%s\">Next</a>\n", cfg.spamcgi_url, cgi.page+1, user, mailfrom, subject);

         if(cgi.page < n_spam/cfg.page_len && n_spam > cfg.page_len)
            printf(" <a href=\"%s?page=%d&user=%s&from=%s&subject=%s\">Last</a><p>\n", cfg.spamcgi_url, n_spam/cfg.page_len, user, mailfrom, subject);

      }

   }

   /* purge selected message(s) */

   if(method == M_POST){
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

      /* fix username */
      if(admin_user == 1)
         snprintf(user, SMALLBUFSIZE-1, "%s", cgi.user);
      else
         snprintf(user, SMALLBUFSIZE-1, "%s", getenv("REMOTE_USER"));

      snprintf(spamqdir, MAXBUFSIZE-1, "%s/%c/%s", USER_QUEUE_DIR, user[0], user);


      p = input;

      while(p){
         memset(m, 0, SMALLBUFSIZE);
         q = strchr(p, '&');
         if(q){
            if(q-p < SMALLBUFSIZE-1)
               memcpy(m, p, q-p);

            p = q+1;
         }
         else {
            snprintf(m, SMALLBUFSIZE-1, "%s", p);
            p = NULL;
         }

         if(m[0] == 's' && m[1] == '.'){
            snprintf(msg, SMALLBUFSIZE-1, "%s/%s", spamqdir, m);
            q = strchr(msg, '=');
            if(q) *q = '\0';

            unlink(msg);
            n++;
         }
      }

      if(input)
         free(input);

      printf("%s: %d.<p>\n<a href=\"%s?user=%s\">%s</a>\n", ERR_CGI_PURGED_MESSAGES, n, cfg.spamcgi_url, user, ERR_CGI_BACK);
   }


   printf("</blockquote>\n</body></html>\n");

   return 0;
}
