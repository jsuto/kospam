/*
 * mydb_export.c, SJ
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
   unsigned char buf[N_SIZE*SEGMENT_SIZE];
   unsigned int pos=0;
   unsigned long x;
   float Nham, Nspam;
   int i=0, j=0, fd;

   if(argc < 2){
      printf("usage: %s <mydb file>\n", argv[0]);
      return 1;
   }

   if(stat(argv[1], &st)){
      printf("cannot stat %s\n", argv[1]);
      return 1;
   }


   for(i=0;i<MAX_MYDB_HASH;i++)
      mhash[i] = NULL;

   gettimeofday(&tv_start, &tz);

   fd = open(argv[1], O_RDONLY);
   if(fd == -1){
      printf("cannot open %s\n", argv[1]);
      return 1;
   }

   read(fd, &x, 4);
   Nham = (float)x;

   read(fd, &x, 4);
   Nspam = (float)x;

   fprintf(stderr, "Nham: %.0f, Nspam: %.0f\n", Nham, Nspam);

   while((x = read(fd, buf, N_SIZE*SEGMENT_SIZE))){
      for(j=0; j<x/N_SIZE; j++){
         addmydb_node(mhash, ((struct mydb_node*)(&buf[j*N_SIZE]))->key, ((struct mydb_node*)(&buf[j*N_SIZE]))->nham, ((struct mydb_node*)(&buf[j*N_SIZE]))->nspam, ((struct mydb_node*)(&buf[j*N_SIZE]))->ts, pos);
         pos++;
      }
   }

   close(fd);

   gettimeofday(&tv_stop, &tz);

   j = 0;
   printf("INSERT INTO %s (token,nham,nspam,timestamp,uid) VALUES ", SQL_TOKEN_TABLE);

   for(i=0;i<MAX_MYDB_HASH;i++){
      z = mhash[i];
      while(z != NULL){

         if(j > 0) printf(",");

         printf("(%llu,%d,%d,%ld,0)", z->key, z->nham, z->nspam, z->ts);

         z = z->r;
         j++;
      }
   }

   close_mydb(mhash);

   printf("\n");

   return 0;
}
