/*
 * spamcgi.c, 2007.09.25, SJ
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


FILE *cgiIn;
char *input=NULL;
struct __config cfg;
int page=0;

char *extract(char *row, int ch, char *s, int size);
int deliver_message(char *dir, char *message, struct __config cfg);
void errout(char *input, char *s);


/*
 * get user id from user table
 */

unsigned long get_uid_from_username(char *username){
   unsigned long uid=0;
   char buf[SMALLBUFSIZE];

   snprintf(buf, SMALLBUFSIZE-1, "SELECT uid FROM %s WHERE username='%s'", SQL_USER_TABLE, username);

#ifdef HAVE_MYSQL
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row) uid = atol(row[0]);
         mysql_free_result(res);
      }
   }
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW) uid = sqlite3_column_int(pStmt, 0);
   }
   sqlite3_finalize(pStmt);
#endif

   return uid;
}


/*
 * extract from and subject from the given message
 */

void scan_message(char *data, char *from, char *subj){
   char *p, buf[MAXBUFSIZE];
   int i=0;

   strncpy(subj, "no subject", SMALLBUFSIZE-1);
   strncpy(from, "no from", SMALLBUFSIZE-1);

   p = data;
   do {
      p = split(p, '\n', buf, MAXBUFSIZE-1);

      if(strncmp(buf, "Subject:", 8) == 0 && strlen(buf) > 11){
         strncpy(subj, buf+9, SMALLBUFSIZE-1);
         i++;
      }
      if(strncmp(buf, "From:", 5) == 0 && strlen(buf) > 10){
         strncpy(from, buf+6, SMALLBUFSIZE-1);
         i++;
      }

      if(i >= 2)
         break;
   } while (p);

   for(i=0; i<strlen(from); i++){
      if(from[i] == '<')
         from[i] = '[';

      if(from[i] == '>')
         from[i] = ']';
   }

}

/*
 * show message list
 */

int print_spam_messages(unsigned long uid, int page){
   int n_spam=0, n_msgs=0;
   double spam_total_size = 0;
   struct tm *t;
   char buf[SMALLBUFSIZE], from[SMALLBUFSIZE], subj[SMALLBUFSIZE], date[SMALLBUFSIZE], *id, *data;
   time_t clock;

   /* get total messages and size */

   snprintf(buf, SMALLBUFSIZE-1, "SELECT COUNT(*), SUM(LENGTH(data)) FROM %s WHERE is_spam=1 AND uid=%ld", SQL_QUEUE_TABLE, uid);

#ifdef HAVE_MYSQL
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            n_spam = atol(row[0]);
            spam_total_size = atof(row[1]);
         }
         mysql_free_result(res);
      }
   }
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         n_spam = sqlite3_column_int(pStmt, 0);
         spam_total_size = sqlite3_column_double(pStmt, 1);
      }
   }
   sqlite3_finalize(pStmt);
#endif

   printf("%s: %d (%.0f bytes)<p>\n", ERR_CGI_NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE, n_spam, spam_total_size);


   /* now show the message list */

   printf("<table border=\"0\">\n");
   printf("<tr align=\"middle\"><th>&nbsp;</th><th>%s</th><th>%s</th><th>%s</th><th>&nbsp;</th></tr>\n", CGI_DATE, CGI_FROM, CGI_SUBJECT);


   snprintf(buf, SMALLBUFSIZE-1, "SELECT ts, id, data FROM %s WHERE is_spam=1 AND uid=%ld ORDER by ts DESC LIMIT %d,%d", SQL_QUEUE_TABLE, uid, cfg.page_len*(page-1), cfg.page_len);

#ifdef HAVE_MYSQL
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         while((row = mysql_fetch_row(res))){

            clock = atol(row[0]);
            id = (char*)row[1];
            data = (char*)row[2];
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      while(sqlite3_step(pStmt) == SQLITE_ROW){

            clock = sqlite3_column_int(pStmt, 0);
            id = (char *)sqlite3_column_blob(pStmt, 1);
            data = (char *)sqlite3_column_blob(pStmt, 2);

#endif

            memset(subj, 0, SMALLBUFSIZE);
            memset(from, 0, SMALLBUFSIZE);

            scan_message(data, from, subj);

            t = localtime(&clock);
            snprintf(date, SMALLBUFSIZE-1, "%d.%02d.%02d. %02d:%02d:%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

            if(strlen(subj) > MAX_CGI_SUBJECT_LEN){
               subj[MAX_CGI_SUBJECT_LEN] = '\0';
               strncat(subj, " ...", SMALLBUFSIZE-1);
            }

            if(strlen(from) > MAX_CGI_FROM_LEN){
               from[MAX_CGI_FROM_LEN] = '\0';
               strncat(from, " ...", SMALLBUFSIZE-1);
            }

            n_msgs++;

               if(!(n_msgs % 2))
                  printf("<tr valign=\"top\">\n<td>%d.</td><td>%s</td><td>%s</td>\n<td><a href=\"%s?id=%s\">%s</a></td>\n<td><input type=\"checkbox\" name=\"%s\"></td>\n</tr>\n", cfg.page_len*(page-1) + n_msgs, date, from, cfg.spamcgi_url, id, subj, id);
               else
                  printf("<tr valign=\"top\">\n<td class=\"odd\">%d.</td><td class=\"odd\">%s</td><td class=\"odd\">%s</td>\n<td class=\"odd\"><a href=\"%s?id=%s\">%s</a></td>\n<td class=\"odd\"><input type=\"checkbox\" name=\"%s\"></td>\n</tr>\n", cfg.page_len*(page-1) + n_msgs, date, from, cfg.spamcgi_url, id, subj, id);

   #ifdef HAVE_MYSQL
         }
         mysql_free_result(res);
   #endif
   #ifdef HAVE_SQLITE3
      sqlite3_finalize(pStmt);
   #endif
      }
   }

   printf("</table><p>\n");

   return n_spam;
}


/*
 * remove the given message
 */

void remove_messages(unsigned long uid, char *id){
   char buf[SMALLBUFSIZE];

   snprintf(buf, SMALLBUFSIZE-1, "DELETE FROM %s WHERE is_spam=1 AND uid=%ld AND id='%s'", SQL_QUEUE_TABLE, uid, id);

#ifdef HAVE_MYSQL
   mysql_real_query(&mysql, buf, strlen(buf));
#endif
#ifdef HAVE_SQLITE3
#endif

}


/*
 * show the given message
 */

void show_message(unsigned long uid, char *id){
   char *data=NULL, buf[SMALLBUFSIZE];
   int i;

   snprintf(buf, SMALLBUFSIZE-1, "SELECT data FROM %s WHERE uid=%ld AND id='%s'", SQL_QUEUE_TABLE, uid, id);

#ifdef HAVE_MYSQL
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            data = (char*)row[0];
            for(i=0; i<strlen(data); i++){
               switch(data[i]){
                  case '<': printf("&lt;");
                         break;

                  case '>': printf("&gt;");
                         break;

                  default:
                         printf("%c", data[i]);
                         break;

               };

            }
         }
         mysql_free_result(res);
      }
      else printf("%s: %s\n", ERR_CGI_CANNOT_OPEN, id);
   }
#endif
#ifdef HAVE_SQLITE3
   /* FIXME */
#endif

}

int main(){
   char *p, *q, *r, m[SMALLBUFSIZE], spamqdir[MAXBUFSIZE];
   int clen=0, method=M_UNDEF, n=0, n_spam=0;
   unsigned long uid=0;


   cgiIn = stdin;

   cfg = read_config(CONFIG_FILE);

   printf("Content-type: text/html\n\n");

   printf("<html>\n<title>%s</title>\n<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n<body bgcolor=white text=darkblue vlink=#AC003A>\n<blockquote>\n", CGI_SPAM_QUARANTINE);

   printf("<h1>%s</h1>\n", CGI_SPAM_QUARANTINE);

//#ifdef HAVE_USERDB
   printf("\n\n<center>%s <a href=\"%s\">%s</a> <a href=\"%s\">%s</a> <a href=\"%s\">%s</a></center>\n\n\n", CGI_SPAM_QUARANTINE, cfg.usercgi_url, CGI_USER_PREF, cfg.statcgi_url, CGI_PERSONAL_STAT, cfg.trainlogcgi_url, CGI_TRAIN_LOG);
//#endif

   printf("<script type=\"text/javascript\">\n\nfunction mark_all(x){\n   var i;\n   var len = document.forms[0].elements.length;\n\n   for(i=0; i<len; i++)\n      document.forms[0].elements[i].checked = x;\n}\n\n</script>\n\n");


   /* check request method */

   if((p = getenv("REQUEST_METHOD"))){
      if(strcmp(p, "GET") == 0)
         method = M_GET;

      if(strcmp(p, "POST") == 0)
         method = M_POST;
   }

   if(!getenv("REMOTE_USER"))
      errout(input, ERR_CGI_NOT_AUTHENTICATED);


   /* connect to database */

#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      errout(input, ERR_MYSQL_CONNECT);
#endif
#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc)
      errout(input, ERR_SQLITE3_OPEN);
#endif

   /* determine user id */

   uid = get_uid_from_username(getenv("REMOTE_USER"));

   snprintf(spamqdir, MAXBUFSIZE-1, "%s/%s", cfg.spam_quarantine_dir, getenv("REMOTE_USER"));

   if(method == M_GET){
      p = getenv("QUERY_STRING");

      /* show selected message ... */

      if(strlen(p) > 30){
         if(strncmp(p, "id=", 3) == 0 && strlen(p) == RND_STR_LEN-2+3){
            p += 3;


         /* use a simpler and more elegant solution than a form submit button, 2006.09.01, SJ */

         #ifdef HAVE_USERDB
            printf("<a href=\"%s?delivery=%s\">%s</a><br>\n", cfg.spamcgi_url, p, ERR_CGI_DELIVER_AND_REMOVE);
         #else
            printf("<a href=\"%s?delivery=%s\">%s</a><br>\n", cfg.spamcgi_url, p, ERR_CGI_REMOVE);
         #endif


         #ifdef HAVE_USER_MYSQL
            printf("<a href=\"%s?train=%s\">%s</a><br>\n", cfg.traincgi_url, p, ERR_CGI_DELIVER_AND_TRAIN_AS_HAM);
         #endif

            printf("<br>\n\n<pre>\n");
            show_message(uid, p);
            printf("</pre>\n");
         }

         /* or deliver message */

         else if(strncmp(p, "delivery=", 9) == 0 && strlen(p) == RND_STR_LEN-2+9){
            p += 9;

         /* only remove if we are without LDAP support */

         #ifdef HAVE_USERDB
            if(deliver_message(spamqdir, p, cfg) == OK){

               remove_messages(uid, p);

               printf("%s (%s).<p>\n<a href=\"%s\">Back.</a>\n", ERR_CGI_DELIVERED_AND_REMOVED, p, cfg.spamcgi_url);
            }
            else
               printf("%s (%s)\n", ERR_CGI_DELIVERY_FAILED, p);
         #else

            remove_messages(uid, p);

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

         n_spam = print_spam_messages(uid, page+1);

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
            if(strlen(m) == RND_STR_LEN-2){
               remove_messages(uid, m);
               n++;
            }
         }

      } while(q);

      if(input)
         free(input);

      printf("%s: %d.<p>\n<a href=\"%s\">%s</a>\n", ERR_CGI_PURGED_MESSAGES, n, cfg.spamcgi_url, ERR_CGI_BACK);
   }


   printf("</blockquote>\n</body></html>\n");

#ifdef HAVE_MYSQL
   mysql_close(&mysql);
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(db);
#endif

   return 0;
}
