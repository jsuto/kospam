/*
 * traincgi.c, 2008.02.13, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "misc.h"
#include "decoder.h"
#include "errmsg.h"
#include "messages.h"
#include "cgi.h"
#include "bayes.h"

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
char *input;
int admin_user = 0;


int deliver_message(char *dir, char *message, char *username, struct __config cfg);
struct _state parse_message(char *spamfile, struct __config cfg);

int main(){
   char *p, buf[MAXBUFSIZE], spamqdir[MAXBUFSIZE], qfile[SMALLBUFSIZE], user[SMALLBUFSIZE];
   int i, clen=0, is_spam=0, method=M_UNDEF, train_mode=T_TOE, rounds=1;
   unsigned long now;
   time_t clock;
   struct _state state;
   struct __config cfg;
   struct cgidata cgi;
   struct session_data sdata;

   sdata.num_of_rcpt_to = 1;
   sdata.uid = 0;
   memset(sdata.rcptto[0], 0, MAXBUFSIZE);
   make_rnd_string(&(sdata.ttmpfile[0]));



   cgiIn = stdin;
   input = NULL;

   printf("Content-type: text/html\n\n");

   cfg = read_config(CONFIG_FILE);

   if(!getenv("REMOTE_USER"))
      errout(NULL, ERR_CGI_NOT_AUTHENTICATED);

   p = getenv("REMOTE_USER");
   if(!p)
      errout(input, ERR_CGI_NOT_AUTHENTICATED);

   /* if you are an administrator */
   if(strcmp(p, cfg.admin_user) == 0) admin_user = 1;


   if((p = getenv("REQUEST_METHOD"))){

      if(strcmp(p, "POST") == 0){
         method = M_POST;

         if((p = getenv("CONTENT_LENGTH")))
            clen = atoi(p);

         if(clen < 10) errout(NULL, ERR_CGI_INVALID_REQUEST);

         input = (char *) malloc(clen);
         if(!input) errout(NULL, ERR_CGI_NO_MEMORY);

         if(((int) fread(input, 1, clen, stdin)) != clen) errout(input, ERR_CGI_POST_READ);

         cgi = extract_cgi_parameters(input);
      }

      else if(strcmp(p, "GET") == 0){
         method = M_GET;
         cgi = extract_cgi_parameters(getenv("QUERY_STRING"));
      }
   }

   if(method == M_UNDEF)
      errout(NULL, ERR_CGI_INVALID_METHOD);

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

   /* select uid */

   if(admin_user == 1)
      snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE username='%s'", SQL_USER_TABLE, getenv("REMOTE_USER"));
   else
      snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE username='%s'", SQL_USER_TABLE, cgi.user);

#ifdef HAVE_MYSQL
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]) sdata.uid = atol(row[0]);
         }
         mysql_free_result(res);
      }
   }
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW)
         sdata.uid = sqlite3_column_int(pStmt, 0);
   }

   sqlite3_finalize(pStmt);
#endif

   /* fix uid if this is a shared group, 2007.08.22, SJ */
   if(cfg.group_type == GROUP_SHARED) sdata.uid = 0;


   time(&clock);
   now = clock;

   state = init_state();

   if(method == M_POST){

      // search type

      if(strcmp(cgi.type, "spam") == 0)
         is_spam = 1;

      if(strncmp(p, "message=", strlen("message=")))
         errout(input, ERR_CGI_NO_MESSAGE);

      p += strlen("message=");

      // url decode cgi data

      url_decode(p);
      i = 0;

      do {
         p = split(p, '\n', buf, MAXBUFSIZE-1);
         state = parse(buf, state);

      } while(p);

      if(input)
        free(input);
   }


   /* handle GET request */

   else if(strcmp(p, "GET") == 0){
      cgi = extract_cgi_parameters(getenv("QUERY_STRING"));

      /* fix username */
      if(admin_user == 1)
         snprintf(user, SMALLBUFSIZE-1, "%s", cgi.user);
      else
         snprintf(user, SMALLBUFSIZE-1, "%s", getenv("REMOTE_USER"));

      snprintf(spamqdir, MAXBUFSIZE-1, "%s/%c/%s", USER_QUEUE_DIR, user[0], user);

      if(strlen(cgi.train) > 1){

         /* train as HAM */
         is_spam = 0;

      #ifdef HAVE_USERDB
         if(chdir(spamqdir))
            errout(NULL, ERR_CHDIR);

         i = deliver_message(spamqdir, cgi.train, user, cfg);
         if(i != OK)
            errout(NULL, ERR_CGI_DELIVERY_FAILED);
      #endif

         i = 0;
         snprintf(qfile, SMALLBUFSIZE-1, "%s/%s", spamqdir, cgi.train);
         state = parse_message(qfile, cfg);

         /* remove message */
         printf("unlinking %s<br/>\n", qfile);

         unlink(qfile);

      }
      else
         errout(NULL, ERR_CGI_INVALID_ID);
   }


#ifdef HAVE_MYSQL
   train_message(mysql, sdata, state, rounds, is_spam, train_mode, cfg);
#endif
#ifdef HAVE_SQLITE3
   train_message(db, sdata, state, rounds, is_spam, train_mode, cfg);
#endif


   if(method == M_GET)
   #ifdef HAVE_USERDB
      printf("%s<p>\n<a href=\"%s\">%s</a>\n", ERR_CGI_DELIVERED_REMOVED_AND_TRAINED_AS_HAM, cfg.spamcgi_url, ERR_CGI_BACK);
   #else
      printf("%s<p>\n<a href=\"%s\">%s</a>\n", ERR_CGI_REMOVED_AND_TRAINED_AS_HAM, cfg.spamcgi_url, ERR_CGI_BACK);
   #endif
   else
      printf("<meta http-equiv=\"refresh\" content=\"%d; url=%s\">\n\n%s\n", cfg.relocate_delay, cfg.relocate_url, ERR_CGI_I_TAUGHT);

   return 0;
}
