/*
 * cgi.c, 2008.02.13, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "cgi.h"
#include "messages.h"
#include "config.h"


void errout(char *input, char *s){
   if(input)
      free(input);

   input = NULL;

   printf("%s\n", s);
   exit(1);
}



struct cgidata extract_cgi_parameters(char *data){
   struct cgidata cgi;
   char *p, *q;

   bzero(&(cgi), sizeof(cgi));

   if((p = strstr(data, "id="))){
      snprintf(cgi.id, SMALLBUFSIZE-1, "%s", p+3);
      p = strchr(cgi.id, '&');
      if(p) *p = '\0';
   }

   if((p = strstr(data, "delivery="))){
      snprintf(cgi.delivery, SMALLBUFSIZE-1, "%s", p+9);
      p = strchr(cgi.delivery, '&');
      if(p) *p = '\0';
   }

   if((p = strstr(data, "train="))){
      snprintf(cgi.train, SMALLBUFSIZE-1, "%s", p+6);
      p = strchr(cgi.train, '&');
      if(p) *p = '\0';
   }

   if((p = strstr(data, "user="))){
      snprintf(cgi.user, SMALLBUFSIZE-1, "%s", p+5);
      p = strchr(cgi.user, '&');
      if(p) *p = '\0';
   }

   if((p = strstr(data, "type="))){
      snprintf(cgi.type, SMALLBUFSIZE-1, "%s", p+5);
      p = strchr(cgi.type, '&');
      if(p) *p = '\0';
   }

   if((p = strstr(data, "page="))){
      if((q = strchr(p+5, '&'))) *q = '\0';
      cgi.page = atoi(p+5);
   }

   return cgi;
}


void show_users(char *queuedir, char *spamcgi_url){
   DIR *dh, *dh2;
   struct dirent *de, *de2;
   struct stat st;

   if(queuedir == NULL)
      return;

   if(chdir(queuedir)) return;

   dh = opendir(queuedir);
   if(dh == NULL)
      return;

   while((de = readdir(dh))){
      if(de->d_name[0] != '.'){
         if(stat(de->d_name, &st) == 0){
            if(S_ISDIR(st.st_mode)){
               dh2 = opendir(de->d_name);
               if(dh2){
                  while((de2 = readdir(dh2))){
                     if(de2->d_name[0] != '.')
                        printf("<a href=\"%s?user=%s\">%s</a><br/>\n", spamcgi_url, de2->d_name, de2->d_name);
                  }
                  closedir(dh2);
               } 
            }

         }
      }
   }

   closedir(dh);
}


/*
 * extract from and subject from the given message
 */

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


int check_directory(char *dir, char *username, int page, int page_len, char *spamcgi_url){
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
                  printf("<tr valign=\"top\">\n<td><a href=\"%s?id=%s&user=%s\">%d.</a></td><td>%s</td><td>%s</td>\n<td><a href=\"%s?id=%s&user=%s\">%s</a></td>\n<td><input type=\"checkbox\" name=\"%s\"></td>\n</tr>\n", spamcgi_url, namelist[n]->d_name, username, n_msgs, date, from, spamcgi_url, namelist[n]->d_name, username, subj, namelist[n]->d_name);
               else
                  printf("<tr valign=\"top\">\n<td class=\"odd\"><a href=\"%s?id=%s&user=%s\">%d.</a></td><td class=\"odd\">%s</td><td class=\"odd\">%s</td>\n<td class=\"odd\"><a href=\"%s?id=%s&user=%s\">%s</a></td>\n<td class=\"odd\"><input type=\"checkbox\" name=\"%s\"></td>\n</tr>\n", spamcgi_url, namelist[n]->d_name, username, n_msgs, date, from, spamcgi_url, namelist[n]->d_name, username, subj, namelist[n]->d_name);
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


/*
 * show the given message
 */

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

