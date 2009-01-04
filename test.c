/*
 * test.c, 2008.11.24, SJ
 *
 * test the bayesian decision with a single message
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
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
#endif


int main(int argc, char **argv){
   struct timezone tz;
   struct timeval tv_spam_start, tv_spam_stop;
   struct session_data sdata;
   struct _state state;
   struct __config cfg;
   float spaminess;
   struct stat st;
#ifdef HAVE_SPAMSUM
   char *sum, spamsum_buf[SMALLBUFSIZE];
   unsigned int flags = 0;
#endif

   if(argc < 3){
      fprintf(stderr, "usage: %s <config file> <message> [<uid>]\n", argv[0]);
      exit(1);
   }

   if(stat(argv[2], &st) != 0){
      fprintf(stderr, "%s is not found\n", argv[2]);
      return 0;
   }

   cfg = read_config(argv[1]);

   fprintf(stderr, "locale: %s\n", setlocale(LC_ALL, cfg.locale));

   /*
    * override training mode to make sure we will not train the token database
    * since we are just testing the spam decision against a message, SJ
    */

   cfg.training_mode = 0;
   cfg.initial_1000_learning=0;

   cfg.debug = 1;

   sdata.uid = 0;
   sdata.num_of_rcpt_to = -1;
   memset(sdata.rcptto[0], 0, MAXBUFSIZE);
   snprintf(sdata.ttmpfile, SMALLBUFSIZE-1, "%s", argv[2]);
   state = parse_message(argv[2], sdata, cfg);

   spaminess = DEFAULT_SPAMICITY;
   sdata.Nham = sdata.Nspam = 0;

   if(argc >= 4) sdata.uid = atoi(argv[3]);

   gettimeofday(&tv_spam_start, &tz);

#ifdef HAVE_QCACHE
   spaminess = x_spam_check(&sdata, &state, &cfg);
#else
#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      spaminess = bayes_file(mysql, state, &sdata, &cfg);
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


      spaminess = bayes_file(db, state, &sdata, &cfg);
   }
   sqlite3_close(db);
#endif

#ifdef HAVE_MYDB
   rc = init_mydb(cfg.mydbfile, sdata.mhash, &sdata);
   fprintf(stderr, "using %s. %.0f, %0.f ...\n", cfg.mydbfile, sdata.Nham, sdata.Nspam);
   if(rc == 1){
      spaminess = bayes_file(state, &sdata, &cfg);
   }
   close_mydb(sdata.mhash);
#endif
#endif /* HAVE_QCACHE */

#ifdef MY_TEST
   reverse_ipv4_addr(state.ip);
   if(rbl_list_check("zen.spamhaus.org", state.ip, cfg.verbosity) == 1)
      printf("%s: ZEN=1\r\n", state.ip);
#endif

#ifdef HAVE_LANG_DETECT
   fprintf(stderr, "lang detected: %s\n", check_lang(state.first));
#endif

   free_and_print_list(state.first, 0);
   free_url_list(state.urls);

   gettimeofday(&tv_spam_stop, &tz);

   fprintf(stderr, "%s: %.4f in %ld [ms]\n", argv[2], spaminess, tvdiff(tv_spam_stop, tv_spam_start)/1000);

   fprintf(stderr, "%ld %ld\n", state.c_shit, state.l_shit);

#ifdef HAVE_SPAMSUM
   gettimeofday(&tv_spam_start, &tz);
   flags |= FLAG_IGNORE_HEADERS;
   sum = spamsum_file(argv[2], flags, 0);
   if(sum){
      snprintf(spamsum_buf, SMALLBUFSIZE-1, "spamsum=%d\n", spamsum_match_db(cfg.sig_db, sum, 55));
      free(sum);
      gettimeofday(&tv_spam_stop, &tz);
      fprintf(stderr, "%s in %ld [ms]", spamsum_buf, tvdiff(tv_spam_stop, tv_spam_start)/1000);
   }
#endif

   if(spaminess >= cfg.spam_overall_limit)
      return 1;

   return 0;
}
