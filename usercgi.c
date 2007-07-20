/*
 * usercgi.c, 2007.03.19, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "cgi.h"
#include "user.h"
#include "errmsg.h"
#include "messages.h"
#include "config.h"

FILE *cgiIn;
char *input=NULL;
static char *actions[] = { "quarantine", "junk", "drop" };
struct __config cfg;

char *extract(char *row, int ch, char *s, int size);


int main(){
   char i, *p, *q, *r, m[SMALLBUFSIZE];
   int clen=0, method=M_UNDEF;
   struct userpref u;

   cgiIn = stdin;

   cfg = read_config(CONFIG_FILE);

   printf("Content-type: text/html\n\n");

   printf("<html>\n<title>%s</title>\n<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n<body bgcolor=white text=darkblue vlink=#AC003A>\n<blockquote>\n<h1>%s</h1>\n", CGI_USER_PREF, CGI_USER_PREF);

   printf("\n\n<center><a href=\"%s\">%s</a> %s <a href=\"%s\">%s</a> <a href=\"%s\">%s</a></center>\n\n\n", cfg.spamcgi_url, CGI_SPAM_QUARANTINE, CGI_USER_PREF, cfg.statcgi_url, CGI_PERSONAL_STAT, cfg.trainlogcgi_url, CGI_TRAIN_LOG);

   /* check request method */

   if((p = getenv("REQUEST_METHOD"))){
      if(strcmp(p, "GET") == 0)
         method = M_GET;

      if(strcmp(p, "POST") == 0)
         method = M_POST;
   }

   if(!getenv("REMOTE_USER"))
      errout(input, ERR_CGI_NOT_AUTHENTICATED);


   printf("You are: %s<p>\n", getenv("REMOTE_USER"));


   /* show current user preferences */

   if(method == M_GET){

   #ifdef HAVE_USER_LDAP
      u = ldap_get_entry(cfg.ldap_host, cfg.ldap_base, cfg.ldap_user, cfg.ldap_pwd, cfg.ldap_use_tls, getenv("REMOTE_USER"));
   #endif

   #ifdef HAVE_USER_MYSQL
      u = mysql_get_entry(cfg.mysqlhost, cfg.mysqlport, cfg.mysqlsocket, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, SQL_USER_TABLE, getenv("REMOTE_USER"));
   #endif

      printf("<form action=\"%s\" method=\"post\">\n", getenv("SCRIPT_NAME"));
      printf("<table align=\"center\" border=\"1\">\n");

      printf("<tr><td>E-mail address:</td><td><input type=\"text\" name=\"email\" value=\"%s\"></td></tr>\n", u.email);
      //printf("<tr><td>E-mail address:</td><td>%s</td></tr>\n", u.email);

      printf("<tr><td>Action:</td><td><select name=\"action\">\n");

      for(i=0; i<(sizeof(actions) / sizeof(p)); i++){
         printf("<option value=\"%s\"", actions[(int)i]);

         if(strcmp(u.action, actions[(int)i]) == 0)
            printf(" selected ");

         printf(">%s\n", actions[(int)i]);
      }

      printf("</select></td></tr>\n");

      printf("<tr><td>Page length:</td><td><input type=\"text\" name=\"pagelen\" value=\"%s\" size=\"3\" maxlength=\"3\"> messages</td></tr>\n", u.pagelen);

      printf("<tr><td></td><td><input type=\"submit\" name=\"submit\" value=\"Set\"> <input type=\"reset\" value=\"cancel\"></td></tr>\n");

      printf("</table><p>\n");
      printf("</form>\n</body></html>\n");
   }


   /* update user preferences */


   else if(method == M_POST){
      if((p = getenv("CONTENT_LENGTH")))
         clen = atoi(p);

      input = (char *) malloc(clen);
      if(!input)
         errout(input, ERR_CGI_NO_MEMORY);

      memset(input, 0, clen);

      if(((int) fread(input, 1, clen, stdin)) != clen)
         errout(input, ERR_CGI_POST_READ);


      p = input;

      memset((char *)&u, 0, sizeof(u));

      do {
         q = extract(p, '&', m, SMALLBUFSIZE-1);
         if(q) p = q;
         else {
            memset(m, 0, SMALLBUFSIZE);
            strncpy(m, p, SMALLBUFSIZE-1);
         }

         url_decode(m);

         r = strchr(m, '=');
         if(r){
            *r = '\0';
            if(strcmp(m, "email") == 0)
               strncpy(u.email, ++r, SMALLBUFSIZE-1);
            else if(strcmp(m, "action") == 0)
               strncpy(u.action, ++r, SMALLBUFSIZE-1);
            else if(strcmp(m, "pagelen") == 0)
               strncpy(u.pagelen, ++r, SMALLBUFSIZE-1);
         }

      } while(q);

      if(input)
         free(input);

      /* action and email address should be verified! */

   #ifdef HAVE_USERDB
      #ifdef HAVE_USER_LDAP
         i = ldap_set_entry(cfg.ldap_host, cfg.ldap_base, cfg.ldap_user, cfg.ldap_pwd, cfg.ldap_use_tls, getenv("REMOTE_USER"), u);
      #endif

      #ifdef HAVE_USER_MYSQL
         i = mysql_set_entry(cfg.mysqlhost, cfg.mysqlport, cfg.mysqlsocket, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, SQL_USER_TABLE, getenv("REMOTE_USER"), u);
      #endif

      if(i == OK)
         printf("<meta http-equiv=\"refresh\" content=\"%d; url=http://%s%s\">\n\n%s\n</body></html>\n",
              cfg.relocate_delay, getenv("SERVER_NAME"), getenv("SCRIPT_NAME"), ERR_CGI_USERPREF_UPDATED);
      else
         errout(NULL, ERR_CGI_USERPREF_UPDATE_FAILED);
   #endif

   }
   else
      errout(NULL, ERR_CGI_INVALID_METHOD);


   return 0;
}
