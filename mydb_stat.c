/*
 * mydb_stat.c, 2007.10.22, SJ
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
#include "mydb.h"
#include "config.h"


unsigned long now;
unsigned int NHAM, NSPAM;
struct timezone tz;
struct timeval tv_start, tv_stop;



int main(int argc, char **argv){
   struct stat st;
   struct mydb_node *z;
   unsigned char buf[N_SIZE*SEGMENT_SIZE];
   unsigned int pos=0;
   unsigned int ntokens=0, ham_hapax=0, spam_hapax=0, old_tokens=0, _15_obsoleted_tokens=0, _60_obsoleted_tokens=0;
   unsigned long x;
   time_t cclock;
   int i=0, j=0, fd;

   if(argc < 2){
      printf("usage: %s <mydb file>\n", argv[0]);
      return 1;
   }

   time(&cclock);
   now = cclock;

   for(i=0;i<MAX_MYDB_HASH;i++)
      mhash[i] = NULL;

   gettimeofday(&tv_start, &tz);

   if(stat(argv[1], &st)){
      printf("cannot stat %s\n", argv[1]);
      return 1;
   }

   fd = open(argv[1], O_RDONLY);
   if(fd == -1){
      printf("cannot open %s\n", argv[1]);
      return 1;
   }

   read(fd, &x, 4);
   Nham = (float)x;
   read(fd, &x, 4);
   Nspam = (float)x;


   while((x = read(fd, buf, N_SIZE*SEGMENT_SIZE))){
      for(j=0; j<x/N_SIZE; j++){
         addmydb_node(mhash, ((struct mydb_node*)(&buf[j*N_SIZE]))->key, ((struct mydb_node*)(&buf[j*N_SIZE]))->nham, ((struct mydb_node*)(&buf[j*N_SIZE]))->nspam, ((struct mydb_node*)(&buf[j*N_SIZE]))->ts, pos);
         pos++;
      }
   }

   close(fd);

   gettimeofday(&tv_stop, &tz);


   for(i=0;i<MAX_MYDB_HASH;i++){
      z = mhash[i];
      while(z != NULL){
         ntokens++;

         if(z->nham <= 3 && z->nspam == 0) ham_hapax++;
         if(z->nham == 0 && z->nspam <= 3) spam_hapax++;

         if(z->key < 1) goto NEXT_RECORD;
         if(z->ts < now - _90_DAYS){
            old_tokens++;
            goto NEXT_RECORD;
         }
         if(z->nham + z->nspam <= 1 && z->ts < now - _15_DAYS){
            _15_obsoleted_tokens++;
            goto NEXT_RECORD;
         }
         if(z->nham + z->nspam < 5 && z->ts < now - _60_DAYS){
            _60_obsoleted_tokens++;
            goto NEXT_RECORD;
         }

      NEXT_RECORD:
         z = z->r;
      }
   }

   close_mydb(mhash);


   printf("db size: %ld bytes\nham messages: %0.f\nspam messages: %.0f\n", st.st_size, Nham, Nspam);
   printf("number of tokens: %d\nham hapaxes: %d\nspam hapaxes: %d\nold tokens: %d\n", ntokens, ham_hapax, spam_hapax, old_tokens);
   printf("obsolete tokens: 15: %d, 60: %d\n", _15_obsoleted_tokens, _60_obsoleted_tokens);

   return 0;
}
