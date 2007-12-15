/*
 * train.c, 2007.12.15, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "sql.h"
#include "config.h"

extern char *optarg;
extern int optind;

#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL mysql;
   void my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode);
#endif
#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;
   int rc;
   void my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode);
#endif
#ifdef HAVE_MYDB
   #include "mydb.h"
   char *mydbfile=NULL;
   int rc;
   int my_walk_hash(char *mydbfile, struct mydb_node *xhash[MAX_MYDB_HASH], int ham_or_spam, struct node *qhash[MAXHASH], int train_mode);
#endif


int main(int argc, char **argv){
   char buf[MAXBUFSIZE];
   char *configfile=CONFIG_FILE, *messagefile=NULL;
   int i, m, addspam=0, addham=0, is_spam=0;
   unsigned long cnt;
   struct _state state;
   struct _token *P, *Q;
   struct __config cfg;
   struct node *tokens[MAXHASH];
   qry QRY;
   FILE *F;

   QRY.uid = 0;

   while((i = getopt(argc, argv, "c:S:H:u:m:h")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'S' :
                    addspam = 1;
                    is_spam = 1;
                    messagefile = optarg;
                    break;

         case 'H' :
                    addham = 1;
                    messagefile = optarg;
                    break;

         case 'u' :
                    if(atol(optarg) > 0) QRY.uid = atol(optarg);
                    break;

      #ifdef HAVE_MYDB
         case 'm' :
                    mydbfile = optarg;
                    break;
      #endif

         case 'h' :
         default  : 
                    __fatal(TRAINUSAGE);
       }
   }

   if(addham == 0 && addspam == 0)
      __fatal(ERR_INVALID_MODE ": " TRAINUSAGE);

   if(addham == 1 && addspam == 1)
      __fatal(ERR_INVALID_MODE ": " TRAINUSAGE);


   if(messagefile == NULL)
       __fatal(TRAINUSAGE);

   cfg = read_config(configfile);

#ifdef HAVE_MYDB
   if(mydbfile == NULL)
      mydbfile = cfg.mydbfile;
#endif

   state = init_state();

   /* read message file */

   F = fopen(messagefile, "r");
   if(!F)
      __fatal(ERR_OPEN_MESSGAE_FILE);

   while(fgets(buf, MAXBUFSIZE-1, F))
      state = parse(buf, state);


#ifdef HAVE_MYSQL
   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      if(state.first)
         free_and_print_list(state.first, 0);

      __fatal(ERR_MYSQL_CONNECT);
   }

   QRY.mysql = mysql;
#endif
#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc)
      __fatal(ERR_SQLITE3_OPEN);

   QRY.db = db;   
#endif
#ifdef HAVE_MYDB
   rc = init_mydb(mydbfile, mhash);
   if(rc != 1)
      __fatal(ERR_MYDB_OPEN);
#endif

   inithash(tokens);

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
      if(QRY.sockfd == -1){
         goto ENDE;
      }
   #endif

   #ifdef HAVE_MYDB
      my_walk_hash(mydbfile, mhash, is_spam, tokens, T_TOE);
   #else
      my_walk_hash(QRY, is_spam, SQL_TOKEN_TABLE, tokens, T_TOE);
   #endif

      if(QRY.sockfd != -1) close(QRY.sockfd);


      /* update the t_misc table */

   #ifdef HAVE_MYSQL
      if(is_spam == 1)
         snprintf(buf, MAXBUFSIZE-1, "update %s set nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
      else
         snprintf(buf, MAXBUFSIZE-1, "update %s set nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);

      mysql_real_query(&mysql, buf, strlen(buf));
   #endif
   #ifdef HAVE_SQLITE3
      if(is_spam == 1)
         snprintf(buf, MAXBUFSIZE-1, "update %s set nspam=nspam+1 WHERE uid='%ld'", SQL_MISC_TABLE, QRY.uid);
      else
         snprintf(buf, MAXBUFSIZE-1, "update %s set nham=nham+1 WHERE uid='%ld'", SQL_MISC_TABLE, QRY.uid);

      sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);
   #endif

   }



   printf("%s\n", ERR_TRAIN_DONE);

#ifdef HAVE_QCACHE
ENDE:
#endif

   clearhash(tokens);
#ifdef HAVE_MYSQL
   mysql_close(&mysql);
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(db);
#endif
#ifdef HAVE_MYDB
   close_mydb(mhash);
#endif

   return 0;
}
