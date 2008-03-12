/*
 * tune.c, 2008.02.15, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#include <sysexits.h>
#include <math.h>
#include "misc.h"
#include "bayes.h"
#include "errmsg.h"
#include "messages.h"
#include "sql.h"
#include "black.h"
#include "smtpcodes.h"
#include "tune.h"
#include "config.h"

#include "mydb.h"

struct mydb_node *mhash[MAX_MYDB_HASH];
struct c_res bayes_file(struct mydb_node *mhash[MAX_MYDB_HASH], char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);

extern char *optarg;
extern int optind;

struct __config cfg;
struct session_data sdata;
struct c_res result;


struct entry e[ARRAY_LEN];

void progressbar(unsigned int cur, unsigned int top){
   uint i, ndots = ceil(70.0 * cur / top);

   if (ndots < 1) ndots = 1;

   printf("\r%3u [", cur);
   for(i=0; i<ndots; i++)
      printf(".");
 
   for(i=ndots; i<70; i++)
      printf(" ");

   printf("]");

   fflush(stdout);
}


void run_tests(struct _state state, int is_spam){
   int i, j, k;

   k = 0;
   for(i=0; i<sizeof(exclusion_radius)/sizeof(double); i++){
      cfg.exclusion_radius = exclusion_radius[i];

      for(j=0; j<sizeof(robs)/sizeof(double); j++){
         cfg.rob_s = robs[j];

         e[k].robs = robs[j];
         e[k].radius = exclusion_radius[i];

         cfg.esf_h = 1;
         cfg.esf_s = 1;

         result = bayes_file(mhash, sdata.ttmpfile, state, sdata, cfg);

         //printf("radius: %.2f, robs: %.2f, spaminess: %.4f, is_spam: %d\n", exclusion_radius[i], robs[j], result.spaminess, is_spam);
         //printf("radius: %.2f, robs: %.2f, spaminess: %.4f, is_spam: %d\n", e[k].radius, e[k].robs, result.spaminess, is_spam);

         if(is_spam == 1 && result.spaminess < cfg.spam_overall_limit) e[k].fn++;
         if(is_spam == 0 && result.spaminess >= cfg.spam_overall_limit) e[k].fp++;
   
         k++;      
      }
   }

}


int read_mbox(char *mailbox, int is_spam){
   FILE *f;
   char buf[MAXBUFSIZE];
   int is_match, tot_msgs=0;
   struct _state state;

   printf("reading %s...\n", mailbox);

   f = fopen(mailbox, "r");
   if(!f) return -1;

   state = init_state();

   while(fgets(buf, MAXBUFSIZE-1, f)){
      is_match = 0;

      if(buf[0] == 'F' && buf[1] == 'r' && buf[2] == 'o' && buf[3] == 'm' && buf[4] == ' '){
         tot_msgs++;

         if(state.first){
            run_tests(state, is_spam);
            progressbar(tot_msgs, 2200);
            free_and_print_list(state.first, 0);
         }

         state = init_state();

         continue;
      }

      state = parse(buf, state);

   }

   /* free the last message */

   if(state.first){
      run_tests(state, is_spam);
      free_and_print_list(state.first, 0);
   }

   fclose(f);

   return 0;
}

int main(int argc, char **argv){
   int i, fp, fn, j, k, l, x, y;
   char *configfile=CONFIG_FILE, *hamtest=NULL, *spamtest=NULL;


   while((i = getopt(argc, argv, "c:H:S:")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'H' :
                    hamtest = optarg;
                    break;

         case 'S' :
                    spamtest = optarg;
                    break;

         default  : 
                    break;
       }
   }

   if(!hamtest || !spamtest){
      fprintf(stderr, "usage: %s -c <config file> -H <ham mbox> -S <spam mbox>\n", argv[0]);
      return 1;
   }

   cfg = read_config(configfile);


   if(init_mydb(cfg.mydbfile, mhash) != 1){
      fprintf(stderr, "cannot open: %s\n", cfg.mydbfile);
      return -1;
   }

   sdata.uid = 0;
   sdata.num_of_rcpt_to = 1;
   memset(sdata.rcptto[0], 0, MAXBUFSIZE);
   //memset(whitelistbuf, 0, SMALLBUFSIZE);

   read_mbox(hamtest, 0); printf("\n");
   read_mbox(spamtest, 1); printf("\n");


   close_mydb(mhash);

   fp = 1000;
   for(i=0; i<ARRAY_LEN; i++){
      printf("radius: %.4f, robs: %.4f, fp: %d, fn: %d\n", e[i].radius, e[i].robs, e[i].fp, e[i].fn);
      if(e[i].fp < fp) fp = e[i].fp;
   }

   fn = 1000; k = 0;
   for(i=0; i<ARRAY_LEN; i++){
      if(e[i].fp == fp && e[i].fn < fn){ fn = e[i].fn; k = i; }
   }

   printf("radius: %.4f, robs: %.4f\n", e[k].radius, e[k].robs);
   return 0;
}
