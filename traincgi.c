/*
 * traincgi.c, 2007.06.07, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <time.h>
#include <unistd.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "cgi.h"
#include "config.h"

FILE *cgiIn, *f, *F;
char *input;
MYSQL mysql;
MYSQL_RES *res;
MYSQL_ROW row;
struct node *tokens[MAXHASH];

void my_walk_hash(MYSQL mysql, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], unsigned int uid, int train_mode);
int deliver_message(char *dir, char *message, struct __config cfg);

int main(){
   char *p, *q, *r, *t, buf[MAXBUFSIZE], puf[SMALLBUFSIZE];
   char spamqdir[MAXBUFSIZE], savedfile[SMALLBUFSIZE], qfile[SMALLBUFSIZE], ID[RND_STR_LEN+1]="";
   int i, m, clen=0, is_spam=0, method=M_UNDEF, train_mode=T_TOE;
   unsigned long cnt, uid=0, now;
   struct _state state;
   struct _token *P, *Q;
   struct __config cfg;

   cgiIn = stdin;
   input = NULL;

   printf("Content-type: text/html\n\n");

   cfg = read_config(CONFIG_FILE);

   if(!getenv("REMOTE_USER"))
      errout(NULL, ERR_CGI_NOT_AUTHENTICATED);

   snprintf(spamqdir, MAXBUFSIZE-1, "%s/%s", cfg.spam_quarantine_dir, getenv("REMOTE_USER"));


   if((p = getenv("REQUEST_METHOD"))){

      if(strcmp(p, "POST") == 0)
         method = M_POST;

      else if(strcmp(p, "GET") == 0)
         method = M_GET;
   }

   if(method == M_UNDEF)
      errout(NULL, ERR_CGI_INVALID_METHOD);


   mysql_init(&mysql);
   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      errout(NULL, ERR_MYSQL_CONNECT);


   /* select uid */

   snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE username='%s'", cfg.mysqlusertable, getenv("REMOTE_USER"));

   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]) uid = atol(row[0]);
         }
         mysql_free_result(res);
      }
   }


   time(&now);

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

      if(cfg.save_trained_emails == 1){
         if(is_spam == 0)
            create_ham_or_spam_path(cfg.saved_ham_path, savedfile, "ham");
         else
            create_ham_or_spam_path(cfg.saved_spam_path, savedfile, "spam");

         F = fopen(savedfile, "w+");
      }


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
      if(strncmp(p, "train=", 6) == 0 && strlen(p) == 36+DATE_STR_LEN){
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
         f = fopen(qfile, "r");
         if(f){
            while(fgets(buf, MAXBUFSIZE-1, f)){
               state = parse(buf, state);

               p = buf;
               do {
                  p = split(p, '\n', puf, SMALLBUFSIZE-1);

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

                  if(strlen(ID) > 2 && strncmp(puf, cfg.clapf_header_field, strlen(cfg.clapf_header_field)) == 0){
                     if(strncmp(puf + strlen(cfg.clapf_header_field), "TUM", 3) == 0)
                        train_mode = T_TUM;
                  }

               } while(p);

            }
            fclose(f);

            /* remove message */
            unlink(qfile);

         }
         else
            errout(NULL, ERR_CGI_CANNOT_OPEN);
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


      my_walk_hash(mysql, is_spam, cfg.mysqltokentable, tokens, uid, train_mode);


      /* update the t_misc table */

      if(is_spam == 1)
         snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET update_cdb=1, nspam=nspam+1", cfg.mysqlmisctable);
      else
         snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET update_cdb=1, nham=nham+1", cfg.mysqlmisctable);

      mysql_real_query(&mysql, buf, strlen(buf));


      /* fix ID if we have to */

      p = strchr(ID, '\r'); if(p) *p = '\0';
      p = strchr(ID, '\n'); if(p) *p = '\0';

      /* add entry to t_train_log table, 2007.05.21, SJ */

      snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (uid, ts, msgid, is_spam) VALUES(%ld, %ld, '%s', %d)", cfg.mysqltraininglogtable, uid, now, ID, is_spam);
      mysql_real_query(&mysql, buf, strlen(buf));


      mysql_close(&mysql);
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
