/*
 * spamcgi.c, 2007.10.04, SJ
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
#include "config.h"

FILE *cgiIn;
char *input=NULL;
struct __config cfg;
int page=0;

char *extract(char *row, int ch, char *s, int size);
int deliver_message(char *dir, char *message, struct __config cfg);
void errout(char *input, char *s);



void scan_message(char *dir, char *message, char *from, char *subj){
   FILE *F;
   char m[SMALLBUFSIZE], *p=NULL;
   int i=0;

   snprintf(m, SMALLBUFSIZE-1, "%s/%s", dir, message);

   snprintf(from, SMALLBUFSIZE-1, "no sender");
   snprintf(subj, SMALLBUFSIZE-1, "no subject");

   F = fopen(m, "r");
   if(F){
      while(fgets(m, SMALLBUFSIZE-1, F)){
         if(strncmp(m, "Subject:", 8) == 0 && strlen(m) > 10){
            p = m + 9;
            strncpy(subj, p, SMALLBUFSIZE-1);
            i++;
         }
         if(strncmp(m, "From:", 5) == 0 && strlen(m) > 10){
            p = m + 6;
            strncpy(from, p, SMALLBUFSIZE-1);
            i++;
         }

         if(i >= 2)
            break;
      }
      fclose(F);
   }

   for(i=0; i<strlen(from); i++){
      if(from[i] == '<')
         from[i] = '[';

      if(from[i] == '>')
         from[i] = ']';
   }

}

int check_directory(char *dir, int page_len){
   int n=0, n_msgs=0, n_spam=0;
   DIR *dh;
   struct dirent **namelist;
   struct stat st;
   struct tm *t;
   char from[SMALLBUFSIZE], subj[SMALLBUFSIZE], date[SMALLBUFSIZE];
   float spam_total_size = 0;
   
   if(dir == NULL)
      return 0;

   n = scandir(dir, &namelist, 0, alphasort);
   if(n == -1)
      return 0;

   n_msgs = n;

   dh = opendir(dir);
   if(dh == NULL)
      return 0;

   if(chdir(dir) == 0){
      while(n--){
         if(strlen(namelist[n]->d_name) == RND_STR_LEN-2+2 && namelist[n]->d_name[0] == 's' && namelist[n]->d_name[1] == '.'){
            n_spam++;
            if(stat(namelist[n]->d_name, &st) == 0)
               spam_total_size += st.st_size;
         }
      }
   }
   else
      return 0;

   n = n_msgs;

   n_msgs = 0;

   printf("%s: %d (%.0f bytes)<p>\n", ERR_CGI_NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE, n_spam, spam_total_size);
      
   printf("<table border=\"0\">\n");
   printf("<tr align=\"middle\"><th>&nbsp;</th><th>%s</th><th>%s</th><th>%s</th><th>&nbsp;</th></tr>\n", CGI_DATE, CGI_FROM, CGI_SUBJECT);

   while(n--){
      if(strlen(namelist[n]->d_name) == RND_STR_LEN-2+2 && namelist[n]->d_name[0] == 's' && namelist[n]->d_name[1] == '.'){
         memset(subj, 0, SMALLBUFSIZE);
         memset(from, 0, SMALLBUFSIZE);

         n_msgs++;

         scan_message(dir, namelist[n]->d_name, from, subj);

         if(n_msgs > page_len*page && n_msgs <= page_len*(page+1)){
            if(stat(namelist[n]->d_name, &st) == 0){
               t = localtime(&(st.st_mtime));
               snprintf(date, SMALLBUFSIZE-1, "%d.%02d.%02d. %02d:%02d:%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

               if(strlen(subj) > MAX_CGI_SUBJECT_LEN){
                  subj[MAX_CGI_SUBJECT_LEN] = '\0';
                  strncat(subj, " ...", SMALLBUFSIZE-1);
               }

               if(strlen(from) > MAX_CGI_FROM_LEN){
                  from[MAX_CGI_FROM_LEN] = '\0';
                  strncat(from, " ...", SMALLBUFSIZE-1);
               }

               if(!(n_msgs % 2))
                  printf("<tr valign=\"top\">\n<td>%d.</td><td>%s</td><td>%s</td>\n<td><a href=\"%s?id=%s\">%s</a></td>\n<td><input type=\"checkbox\" name=\"%s\"></td>\n</tr>\n", n_msgs, date, from, cfg.spamcgi_url, namelist[n]->d_name, subj, namelist[n]->d_name);
               else
                  printf("<tr valign=\"top\">\n<td class=\"odd\">%d.</td><td class=\"odd\">%s</td><td class=\"odd\">%s</td>\n<td class=\"odd\"><a href=\"%s?id=%s\">%s</a></td>\n<td class=\"odd\"><input type=\"checkbox\" name=\"%s\"></td>\n</tr>\n", n_msgs, date, from, cfg.spamcgi_url, namelist[n]->d_name, subj, namelist[n]->d_name);
            }
         }

      }
      free(namelist[n]);
   }

   free(namelist);

   closedir(dh);

   printf("</table><p>\n");

   return n_spam;
}

void show_message(char *dir, char *message){
   char m[SMALLBUFSIZE];
   int i, len, fd;

   snprintf(m, SMALLBUFSIZE-1, "%s/%s", dir, message);

   fd = open(m, O_RDONLY);
   if(fd == -1)
      printf("%s: %s\n", ERR_CGI_CANNOT_OPEN, m);
   else {
      while((len = read(fd, m, SMALLBUFSIZE)) > 0){
         for(i=0; i<len; i++){
            switch(m[i]){
               case '<': printf("&lt;");
                         break;

               case '>': printf("&gt;");
                         break;

               default:
                         printf("%c", m[i]);
                         break;

            };

         }
      }
   }

}

int main(){
   char *p, *q, *r, m[SMALLBUFSIZE], msg[SMALLBUFSIZE], spamqdir[MAXBUFSIZE];
   int clen=0, method=M_UNDEF, n=0, n_spam=0;

   cgiIn = stdin;

   cfg = read_config(CONFIG_FILE);

   printf("Content-type: text/html\n\n");

   printf("<html>\n<title>%s</title>\n<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n<body bgcolor=white text=darkblue vlink=#AC003A>\n<blockquote>\n", CGI_SPAM_QUARANTINE);

   printf("<h1>%s</h1>\n", CGI_SPAM_QUARANTINE);

   printf("\n\n<center>%s <a href=\"%s\">%s</a> <a href=\"%s\">%s</a> <a href=\"%s\">%s</a></center>\n\n\n", CGI_SPAM_QUARANTINE, cfg.usercgi_url, CGI_USER_PREF, cfg.statcgi_url, CGI_PERSONAL_STAT, cfg.trainlogcgi_url, CGI_TRAIN_LOG);

   printf("<script type=\"text/javascript\">\n\nfunction mark_all(x){\n   var i;\n   var len = document.forms[0].elements.length;\n\n   for(i=0; i<len; i++)\n      document.forms[0].elements[i].checked = x;\n}\n\n</script>\n\n");


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

   snprintf(spamqdir, MAXBUFSIZE-1, "%s/%c/%s", USER_DATA_DIR, *p, p);


   if(method == M_GET){
      p = getenv("QUERY_STRING");

      /* show selected message ... */

      if(strlen(p) > 30){
         if(strncmp(p, "id=", 3) == 0 && strlen(p) == RND_STR_LEN-2+5){
            p += 3;


         #ifdef HAVE_USERDB
            printf("<a href=\"%s?delivery=%s\">%s</a><br>\n", cfg.spamcgi_url, p, ERR_CGI_DELIVER_AND_REMOVE);
         #else
            printf("<a href=\"%s?delivery=%s\">%s</a><br>\n", cfg.spamcgi_url, p, ERR_CGI_REMOVE);
         #endif


         #ifdef HAVE_USER_MYSQL
            printf("<a href=\"%s?train=%s\">%s</a><br>\n", cfg.traincgi_url, p, ERR_CGI_DELIVER_AND_TRAIN_AS_HAM);
         #endif

            printf("<br>\n\n<pre>\n");
            show_message(spamqdir, p);
            printf("</pre>\n");
         }

         /* or deliver message */

         else if(strncmp(p, "delivery=", 9) == 0 && strlen(p) == RND_STR_LEN-2+11){
            p += 9;

         /* only remove if we are without LDAP support */

         #ifdef HAVE_USERDB
            if(deliver_message(spamqdir, p, cfg) == OK){

               snprintf(m, SMALLBUFSIZE-1, "%s/%s", spamqdir, p);
               unlink(m);

               printf("%s (%s).<p>\n<a href=\"%s\">Back.</a>\n", ERR_CGI_DELIVERED_AND_REMOVED, p, cfg.spamcgi_url);
            }
            else
               printf("%s (%s)\n", ERR_CGI_DELIVERY_FAILED, p);
         #else
            snprintf(m, SMALLBUFSIZE-1, "%s/%s", spamqdir, p);
            unlink(m);

            printf("%s (%s).<p>\n<a href=\"%s\">Back.</a>\n", ERR_CGI_REMOVED, p, cfg.spamcgi_url);
         #endif

         }

         else
            printf("%s (%s)\n", ERR_CGI_INVALID_ID, p);
      }

      /* ... or scan directory */

      else {
         if(strncmp(p, "page=", 5) == 0){
            p += 5;
            if(atoi(p) > 0)
               page = atoi(p);
         }

         printf("<form action=\"%s\" name=\"aaa1\" method=\"post\">\n", cfg.spamcgi_url);
         printf("<input type=\"hidden\" name=\"topurge\" value=\"1\">\n");

         n_spam = check_directory(spamqdir, cfg.page_len);

         printf("<input type=\"submit\" value=\"%s\"> <input type=\"reset\" value=\"%s\">\n<input type=\"button\" value=\"%s\" onClick=\"mark_all(true)\"></form><p>\n", ERR_CGI_PURGE_SELECTED, ERR_CGI_CANCEL, ERR_CGI_SELECT_ALL);


         /* print first/prev/next/last links, 2006.09.04, SJ */

         if(page > 0){
            printf("<a href=\"%s?page=0\">First</a> <a href=\"%s?page=%d\">Previous</a>\n", cfg.spamcgi_url, cfg.spamcgi_url, page-1);
         }

         if(n_spam >= cfg.page_len*(page+1) && n_spam > cfg.page_len)
            printf(" <a href=\"%s?page=%d\">Next</a>\n", cfg.spamcgi_url, page+1);

         if(page < n_spam/cfg.page_len && n_spam > cfg.page_len)
            printf(" <a href=\"%s?page=%d\">Last</a><p>\n", cfg.spamcgi_url, n_spam/cfg.page_len);

      }

   }

   /* purge selected message(s) */

   if(method == M_POST){
      if((p = getenv("CONTENT_LENGTH")))
         clen = atoi(p);

      input = (char *) malloc(clen);
      if(!input)
         errout(input, ERR_CGI_NO_MEMORY);

      if(((int) fread(input, 1, clen, stdin)) != clen)
         errout(input, ERR_CGI_POST_READ);


      p = input;

      do {
         q = extract(p, '&', m, SMALLBUFSIZE-1);
         if(q) p = q;
         else {
            memset(m, 0, SMALLBUFSIZE);
            strncpy(m, p, SMALLBUFSIZE-1);
         }

         r = strchr(m, '=');
         if(r){
            *r = '\0';
            if(strlen(m) == RND_STR_LEN-2+2){

               /* unlink message and its delivery info file */

               snprintf(msg, SMALLBUFSIZE-1, "%s/%s", spamqdir, m);
               unlink(msg);

               n++;
            }
         }

      } while(q);

      if(input)
         free(input);

      printf("%s: %d.<p>\n<a href=\"%s\">%s</a>\n", ERR_CGI_PURGED_MESSAGES, n, cfg.spamcgi_url, ERR_CGI_BACK);
   }


   printf("</blockquote>\n</body></html>\n");

   return 0;
}
