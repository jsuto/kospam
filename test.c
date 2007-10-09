/*
 * test.c, 2007.10.09, SJ
 *
 * test the bayesian decision with a single message
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "misc.h"
#include "bayes.h"
#include "errmsg.h"
#include "config.h"

#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL mysql;
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   int rc;
#endif

#ifdef HAVE_MYDB
   #include "mydb.h"
   int rc;
#endif

int main(int argc, char **argv){
   double spaminess=DEFAULT_SPAMICITY;
   struct timezone tz;
   struct timeval tv_spam_start, tv_spam_stop;
   struct session_data sdata;
   struct _state state;
   struct __config cfg;

   if(argc < 3){
      fprintf(stderr, "usage: %s <config file> <message> [<uid>]\n", argv[0]);
      exit(1);
   }

   cfg = read_config(argv[1]);

   /*
    * override training mode to make sure we will not train the token database
    * since we are just testing the spam decision against a message, SJ
    */

   cfg.training_mode = 0;
   cfg.initial_1000_learning=0;

   sdata.uid = 0;
   sdata.num_of_rcpt_to = -1;
   memset(sdata.rcptto[0], MAXBUFSIZE, 0);
   state = parse_message(argv[2], cfg);

   if(argc >= 4) sdata.uid = atoi(argv[3]);

   gettimeofday(&tv_spam_start, &tz);

#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      spaminess = bayes_file(mysql, argv[2], state, sdata, cfg);
      mysql_close(&mysql);
   }
   else {
      fprintf(stderr, "cannot connect to database\n");
   }
#endif

#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc){
      fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
   }
   else {
      rc = sqlite3_exec(db, "PRAGMA synchronous = FULL", 0, 0, NULL);
      if(rc != SQLITE_OK)
          fprintf(stderr, "error happened\n");


      spaminess = bayes_file(db, argv[2], state, sdata, cfg);
   }
   sqlite3_close(db);
#endif

#ifdef HAVE_MYDB
   rc = init_mydb(cfg.mydbfile, mhash);
   fprintf(stderr, "using %s...\n", cfg.mydbfile);
   if(rc == 1){
      spaminess = bayes_file(argv[2], state, sdata, cfg);
   }
   close_mydb(mhash);
#endif

   free_and_print_list(state.first, 0);

   gettimeofday(&tv_spam_stop, &tz);

   fprintf(stderr, "%s: %.4f in %ld [ms]\n", argv[2], spaminess, tvdiff(tv_spam_stop, tv_spam_start)/1000);

   if(spaminess >= cfg.spam_overall_limit)
      return 1;

   return 0;
}
