/*
 * test.c, 2008.03.18, SJ
 *
 * test the bayesian decision with a single message
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <clapf.h>

#ifdef HAVE_MYSQL
   MYSQL mysql;
#endif
#ifdef HAVE_SQLITE3
   sqlite3 *db;
   int rc;
#endif
#ifdef HAVE_MYDB
   int rc;
   struct mydb_node *mhash[MAX_MYDB_HASH];
#endif


int main(int argc, char **argv){
   struct timezone tz;
   struct timeval tv_spam_start, tv_spam_stop;
   struct session_data sdata;
   struct _state state;
   struct __config cfg;
   struct c_res result;

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
   memset(sdata.rcptto[0], 0, MAXBUFSIZE);
   state = parse_message(argv[2], cfg);

   result.spaminess = DEFAULT_SPAMICITY;
   result.ham_msg = result.spam_msg = 0;

   if(argc >= 4) sdata.uid = atoi(argv[3]);

   gettimeofday(&tv_spam_start, &tz);

#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      result = bayes_file(mysql, argv[2], state, sdata, cfg);
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
      rc = sqlite3_exec(db, cfg.sqlite3_pragma, 0, 0, NULL);
      if(rc != SQLITE_OK)
          fprintf(stderr, "error happened\n");


      result = bayes_file(db, argv[2], state, sdata, cfg);
   }
   sqlite3_close(db);
#endif

#ifdef HAVE_MYDB
   rc = init_mydb(cfg.mydbfile, mhash);
   fprintf(stderr, "using %s...\n", cfg.mydbfile);
   if(rc == 1){
      result = bayes_file(mhash, argv[2], state, sdata, cfg);
   }
   close_mydb(mhash);
#endif

#ifdef MY_TEST
   reverse_ipv4_addr(state.ip);
   if(rbl_list_check("zen.spamhaus.org", state.ip) == 1)
      printf("%s: ZEN=1\r\n", state.ip);
#endif

#ifdef HAVE_LANG_DETECT
   fprintf(stderr, "lang detected: %s\n", check_lang(state.first));
#endif

   free_and_print_list(state.first, 0);

   gettimeofday(&tv_spam_stop, &tz);

   fprintf(stderr, "%s: %.4f in %ld [ms]\n", argv[2], result.spaminess, tvdiff(tv_spam_stop, tv_spam_start)/1000);

   if(result.spaminess >= cfg.spam_overall_limit)
      return 1;

   return 0;
}
