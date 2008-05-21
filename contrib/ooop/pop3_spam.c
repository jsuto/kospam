#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <clapf.h>

float process_message(char *messagefile, struct __config cfg){
   struct session_data sdata;
   struct _state state;
   struct c_res result;
   struct stat st;
   struct timezone tz;
   struct timeval tv_spam_start, tv_spam_stop;
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

   if(stat(messagefile, &st) != 0)
      return DEFAULT_SPAMICITY;

   result.spaminess = DEFAULT_SPAMICITY;
   result.ham_msg = result.spam_msg = 0;

   cfg.training_mode = 0;
   cfg.initial_1000_learning=0;


   /* these should be imported from user profile */

   sdata.uid = 0;
   sdata.num_of_rcpt_to = -1;
   memset(sdata.rcptto[0], 0, MAXBUFSIZE);
   state = parse_message(messagefile, sdata, cfg);

   /* import over */

   gettimeofday(&tv_spam_start, &tz);

#ifdef HAVE_MYDB
   rc = init_mydb(cfg.mydbfile, mhash, &sdata);
   if(rc == 1)
      result = bayes_file(mhash, messagefile, state, sdata, cfg);
   close_mydb(mhash);
#endif

   free_and_print_list(state.first, 0);
   gettimeofday(&tv_spam_stop, &tz);

   return result.spaminess;
}
