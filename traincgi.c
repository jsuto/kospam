/*
 * traincgi.c, 2007.12.28, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "cgi.h"
#include "sql.h"
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

FILE *cgiIn, *f, *F;
char *input;
struct node *tokens[MAXHASH];

int my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode);
int deliver_message(char *dir, char *message, struct __config cfg);
struct _state parse_message(char *spamfile, struct __config cfg);

int main(){
   char *p, *q, *r, *t, buf[MAXBUFSIZE], puf[SMALLBUFSIZE];
   char spamqdir[MAXBUFSIZE], qfile[SMALLBUFSIZE], ID[RND_STR_LEN+1]="";
   int i, m, clen=0, is_spam=0, method=M_UNDEF, train_mode=T_TOE;
   unsigned long cnt, now;
   time_t clock;
   struct _state state;
   struct _token *P, *Q;
   struct __config cfg;
   qry QRY;

   cgiIn = stdin;
   input = NULL;

   QRY.uid = 0;

   printf("Content-type: text/html\n\n");

   cfg = read_config(CONFIG_FILE);

   if(!getenv("REMOTE_USER"))
      errout(NULL, ERR_CGI_NOT_AUTHENTICATED);

   p = getenv("REMOTE_USER");
   if(!p)
      errout(input, ERR_CGI_NOT_AUTHENTICATED);

   snprintf(spamqdir, MAXBUFSIZE-1, "%s/%c/%s", USER_QUEUE_DIR, *p, p);


   if((p = getenv("REQUEST_METHOD"))){

      if(strcmp(p, "POST") == 0)
         method = M_POST;

      else if(strcmp(p, "GET") == 0)
         method = M_GET;
   }

   if(method == M_UNDEF)
      errout(NULL, ERR_CGI_INVALID_METHOD);

#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      errout(NULL, ERR_MYSQL_CONNECT);

   QRY.mysql = mysql;
#endif
#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc)
      errout(NULL, ERR_SQLITE3_OPEN);

   QRY.db = db;   
#endif

   /* select uid */

   snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE username='%s'", SQL_USER_TABLE, getenv("REMOTE_USER"));

#ifdef HAVE_MYSQL
   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]) QRY.uid = atol(row[0]);
         }
         mysql_free_result(res);
      }
   }
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW)
         QRY.uid = sqlite3_column_int(pStmt, 0);
   }

   sqlite3_finalize(pStmt);
#endif

   /* fix uid if this is a shared group, 2007.08.22, SJ */
   if(cfg.group_type == GROUP_SHARED) QRY.uid = 0;


   time(&clock);
   now = clock;

   inithash(tokens);

   state = init_state();

   if(method == M_POST){

      if((p = getenv("CONTENT_LENGTH")))
         clen = atoi(p);

      if(clen < 10)
         errout(NULL, ERR_CGI_INVALID_REQUEST);

      input = (char *) malloc(clen);
      if(!input)
         errout(NULL, ERR_CGI_NO_MEMORY);

      if(((int) fread(input, 1, clen, stdin)) != clen)
         errout(input, ERR_CGI_POST_READ);

      // search type

      t = input;

      p = strchr(input, '&');
      if(p){
         *p = '\0'; p++;
         if(strcmp(t, "type=spam") == 0)
            is_spam = 1;
      }

      if(strncmp(p, "message=", strlen("message=")))
         errout(input, ERR_CGI_NO_MESSAGE);

      p += strlen("message=");

      // url decode cgi data

      url_decode(p);
      i = 0;

      do {
         p = split(p, '\n', buf, MAXBUFSIZE-1);
         if(cfg.save_trained_emails == 1 && F) fprintf(F, "%s\n", buf);
         state = parse(buf, state);

         t = buf;
         do {
            t = split(t, '\n', puf, SMALLBUFSIZE-1);

            q = strstr(puf, cfg.clapf_header_field);
            if(q){
               r = strchr(puf, ' ');
               if(r){
                  r++;
                  if(strlen(r) >= 30){
                     i++;
                     if(i == 1){
                        snprintf(ID, RND_STR_LEN, "%s", r);
                     }
                  }
               }
            }

         } while(t);

      } while(p);

      if(cfg.save_trained_emails == 1 && F)
        fclose(F);

      if(input)
        free(input);

   }


   /* handle GET request */

   else if(strcmp(p, "GET") == 0){
      p = getenv("QUERY_STRING");

      if(strncmp(p, "train=", 6) == 0 && strlen(p) == 6+MESSAGE_ID_LEN){
         p += 6;

         /* train as HAM */
         is_spam = 0;

      #ifdef HAVE_USERDB
         i = deliver_message(spamqdir, p, cfg);
         if(i != OK)
            errout(NULL, ERR_CGI_DELIVERY_FAILED);
      #endif

         i = 0;
         snprintf(qfile, SMALLBUFSIZE-1, "%s/%s", spamqdir, p);
         state = parse_message(qfile, cfg);

         /* remove message */
         unlink(qfile);

      }
      else
         errout(NULL, ERR_CGI_INVALID_ID);
   }

   /* update tokens in database */

   if(state.first){

      P = state.first;
      cnt = 0;

      while(P != NULL){
         Q = P->r;
         m = 1;

         addnode(tokens, P->str, 0, 0);

         if(P)
            free(P);

         P = Q;

         cnt++;
      }


      QRY.sockfd = -1;

   #ifdef HAVE_QCACHE
      QRY.sockfd = qcache_socket(cfg.qcache_addr, cfg.qcache_port, cfg.qcache_socket);
   #endif

      my_walk_hash(QRY, is_spam, SQL_TOKEN_TABLE, tokens, train_mode);

      if(QRY.sockfd != -1) close(QRY.sockfd);


      /* update the t_misc table */

      if(is_spam == 1)
         snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1", SQL_MISC_TABLE);
      else
         snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1", SQL_MISC_TABLE);

   #ifdef HAVE_MYSQL
      mysql_real_query(&mysql, buf, strlen(buf));
   #endif
   #ifdef HAVE_SQLITE3
      sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);
   #endif

      /* fix ID if we have to */

      p = strchr(ID, '\r'); if(p) *p = '\0';
      p = strchr(ID, '\n'); if(p) *p = '\0';

      /* add entry to t_train_log table, 2007.05.21, SJ */

      snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (uid, ts, msgid, is_spam) VALUES(%ld, %ld, '%s', %d)", SQL_TRAININGLOG_TABLE, QRY.uid, now, ID, is_spam);

   #ifdef HAVE_MYSQL
      mysql_real_query(&mysql, buf, strlen(buf));
      mysql_close(&mysql);
   #endif
   #ifdef HAVE_SQLITE3
      sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);

      sqlite3_close(db);
   #endif

   }

   clearhash(tokens);

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
