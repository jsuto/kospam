/*
 * mydb_stat.c, 2008.05.11, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "parser.h"
#include "mydb.h"
#include "config.h"


unsigned long now;
unsigned int NHAM, NSPAM;
struct timezone tz;
struct timeval tv_start, tv_stop;



int main(int argc, char **argv){
   struct stat st;
   struct mydb_node *z, *mhash[MAX_MYDB_HASH];
   struct session_data sdata;
   unsigned int ntokens=0, old_tokens=0, _15_obsoleted_tokens=0, _60_obsoleted_tokens=0, ham_hapax=0, spam_hapax=0;
   unsigned long long B=0;
   time_t cclock;
   int i=0, rc=0;

   if(argc < 2){
      printf("usage: %s <mydb file> [<key>]\n", argv[0]);
      return 1;
   }

   time(&cclock);
   now = cclock;

   if(argc > 2) B = strtoull(argv[2], NULL, 10);

   gettimeofday(&tv_start, &tz);

   if(stat(argv[1], &st)){
      printf("cannot stat %s\n", argv[1]);
      return 1;
   }

   rc = init_mydb(argv[1], mhash, &sdata);
   if(rc != 1){
      printf("cannot open %s\n", argv[1]);
      return 0;
   }

   gettimeofday(&tv_stop, &tz);

   if(B){
      z = findmydb_node(mhash, B);
      if(z) printf("%llu, %d, %d, %ld (age: %ld), %d\n", z->key, z->nham, z->nspam, z->ts, now-z->ts, z->pos);
      goto ENDE;
   }

   for(i=0;i<MAX_MYDB_HASH;i++){
      z = mhash[i];
      while(z != NULL){
         ntokens++;

         if(z->nham == 1 && z->nspam == 0) ham_hapax++;
         if(z->nham == 0 && z->nspam == 1) spam_hapax++;

         if(z->key < 1) goto NEXT_RECORD;
         if(z->ts < now - _90_DAYS){
            old_tokens++;
            goto NEXT_RECORD;
         }
         if((2*z->nham + z->nspam <= 1) && (z->ts < now - _15_DAYS)){
            _15_obsoleted_tokens++;
            goto NEXT_RECORD;
         }
         if((z->nham + z->nspam < 5) && (z->ts < now - _60_DAYS)){
            _60_obsoleted_tokens++;
            goto NEXT_RECORD;
         }

      NEXT_RECORD:
         z = z->r;
      }
   }

   close_mydb(mhash);


   printf("db size: %ld bytes\nham messages: %0.f\nspam messages: %.0f\n", st.st_size, sdata.Nham, sdata.Nspam);
   printf("number of tokens: %d\n", ntokens);
   printf("obsolete tokens: 15: %d, 60: %d, 90: %d\n", _15_obsoleted_tokens, _60_obsoleted_tokens, old_tokens);
   printf("ham hapaxes: %d, spam hapaxes: %d\n", ham_hapax, spam_hapax);
ENDE:
   close_mydb(mhash);

   return 0;
}
