/*
 * test.c, 2007.07.17, SJ
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

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   #include <mysql.h>
   MYSQL mysql;
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   int rc;
#endif


int main(int argc, char **argv){
   double spaminess;
   struct timezone tz;
   struct timeval tv_spam_start, tv_spam_stop;
   struct session_data sdata;
   struct __config cfg;

   if(argc < 3){
      fprintf(stderr, "usage: %s <config file> <message> [<uid>]\n", argv[0]);
      exit(1);
   }

   cfg = read_config(argv[1]);

   sdata.uid = 0;
   sdata.num_of_rcpt_to = -1;
   memset(sdata.rcptto[0], MAXBUFSIZE, 0);

   if(argc >= 4) sdata.uid = atoi(argv[3]);

   gettimeofday(&tv_spam_start, &tz);

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   mysql_init(&mysql);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      spaminess = bayes_file(mysql, argv[2], sdata, cfg);
      mysql_close(&mysql);
   }
   else {
      spaminess = ERR_BAYES_NO_TOKEN_FILE;
   }
#endif

#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      spaminess = ERR_BAYES_NO_TOKEN_FILE;
   }
   else {
      spaminess = bayes_file(db, argv[2], sdata, cfg);
   }
   sqlite3_close(db);
#endif

#ifdef HAVE_CDB
   spaminess = bayes_file(cfg.tokensfile, argv[2], sdata, cfg);
#endif

   gettimeofday(&tv_spam_stop, &tz);

   fprintf(stderr, "%s: %.4f in %ld [ms]\n", argv[2], spaminess, tvdiff(tv_spam_stop, tv_spam_start)/1000);

   if(spaminess >= cfg.spam_overall_limit)
      return 1;

   return 0;
}
