/*
 * test.c, 2007.06.13, SJ
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
#include "hash_db.h"
#include "config.h"

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
   spaminess = bayes_file(argv[2], sdata, cfg);
   gettimeofday(&tv_spam_stop, &tz);

   fprintf(stderr, "%s: %.4f in %ld [ms]\n", argv[2], spaminess, tvdiff(tv_spam_stop, tv_spam_start)/1000);

   if(spaminess >= cfg.spam_overall_limit)
      return 1;

   return 0;
}
