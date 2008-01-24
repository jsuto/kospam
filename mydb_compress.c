/*
 * mydb_compress.c, 2008.01.23, SJ
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
   struct mydb e;
   unsigned char *p, buf[N_SIZE*SEGMENT_SIZE], lockfile[SMALLBUFSIZE];
   unsigned int pos=0;
   unsigned int ntokens=0, old_tokens=0, _15_obsoleted_tokens=0, _60_obsoleted_tokens=0;
   unsigned long x;
   time_t cclock;
   int i=0, j=0, fd, fdl;

   if(argc < 2){
      printf("usage: %s <mydb file>\n", argv[0]);
      return 1;
   }

   p = strrchr(argv[1], '/');
   if(!p)
      snprintf(lockfile, SMALLBUFSIZE-1, ".%s", argv[1]);
   else {
      *p = '\0';
      snprintf(lockfile, SMALLBUFSIZE-1, "%s/.%s", argv[1], p+1);
      *p = '/';
   }

   printf("using lockfile: %s\n", lockfile);

   fdl = open(lockfile, O_WRONLY|O_CREAT|O_EXCL|O_TRUNC, S_IRUSR|S_IWUSR);
   if(fdl == -1){
      printf("cannot create lockfile: %s\n", lockfile);
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
   write(fdl, &x, 4);

   read(fd, &x, 4);
   Nspam = (float)x;
   write(fdl, &x, 4);


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

         /* skip old or obsoleted tokens */

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

         e.key = z->key;
         e.nham = z->nham;
         e.nspam = z->nspam;
         e.ts = z->ts;
         write(fdl, &e, sizeof(e));

      NEXT_RECORD:
         z = z->r;
      }
   }

   close(fdl);

   close_mydb(mhash);

   if(rename(lockfile, argv[1])) printf("failed to rename %s to %s\n", lockfile, argv[1]);

   return 0;
}
