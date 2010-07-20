/*
 * prepare-sql.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <clapf.h>


#define MAX_HASH 74713

struct aaaa {
   unsigned long long key;
   unsigned int nham;
   unsigned int nspam;
   struct aaaa *r;
};

unsigned long nham=0, nspam=0;

#ifdef HAVE_MYDB
   #include "mydb.h"
   char *f;
#endif

void _inithash(struct aaaa *xhash[MAX_HASH]){
   int i;

   for(i=0;i<MAX_HASH;i++)
      xhash[i] = NULL;
}


void _clearhash(struct aaaa *xhash[MAX_HASH]){
   int i;
   struct aaaa *p, *q;
   time_t cclock;
   unsigned long now;
#ifdef HAVE_SQLITE3
   printf("BEGIN;\n");
#endif
#ifdef HAVE_MYDB
   int fd, n=0;
   struct mydb e;

   fd = open(f, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);

   if(fd != -1){
      write(fd, &nham, 4);
      write(fd, &nspam, 4);
   }
#endif

   time(&cclock);
   now = cclock;

   for(i=0;i<MAX_HASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

      #ifdef HAVE_MYSQL
         printf("INSERT INTO t_token (token, uid, nham, nspam, timestamp) VALUES(%llu, 0, %d, %d, %ld);\n", p->key, p->nham, p->nspam, now);
      #endif
      #ifdef HAVE_SQLITE3
         printf("INSERT INTO t_token (token, nham, nspam, timestamp) VALUES(%llu, %d, %d, %ld);\n", p->key, p->nham, p->nspam, now);
      #endif
      #ifdef HAVE_MYDB
         if(fd != -1 && p->nham + p->nspam > 2){
            e.key = p->key;
            e.nham = p->nham;
            e.nspam = p->nspam;
            e.ts = now;
            write(fd, &e, N_SIZE);
            n++;
         }
      #endif

         q = q->r;
         if(p)
            free(p);
      }
      xhash[i] = NULL;
   }

#ifdef HAVE_SQLITE3
   printf("COMMIT;\n");
#endif
#ifdef HAVE_MYDB
   if(fd != -1) close(fd);

   fprintf(stderr, "put %d records\n", n);
#endif

}


unsigned long _hash(unsigned long long key){
    return key % MAX_HASH;
}

struct aaaa *_makenewnode(struct aaaa *xhash[MAX_HASH], unsigned long long key, unsigned int nham, unsigned int nspam){
   struct aaaa *h;

   if((h = malloc(sizeof(struct aaaa))) == NULL)
      return NULL;

   memset(h, 0, sizeof(struct aaaa));

   h->key = key;
   h->nham = nham;
   h->nspam = nspam;
   h->r = NULL;

   return h;
}


int _addnode(struct aaaa *xhash[MAX_HASH], unsigned long long key, unsigned int nham, unsigned int nspam){
   struct aaaa *p=NULL, *q;

   if(xhash[_hash(key)] == NULL){
      xhash[_hash(key)] = _makenewnode(xhash, key, nham, nspam);
   }
   else {
      q = xhash[_hash(key)];
      while(q != NULL){
         p = q;
         if(q->key == key){
            q->nham += nham;
            q->nspam += nspam;
            return 0;
         }
         else
            q = q->r;
      }
      p->r = _makenewnode(xhash, key, nham, nspam);
   }

   return 1;
}


int main(int argc, char **argv){
   FILE *fham, *fspam;
   char buf[MAXBUFSIZE];
   struct aaaa *tokens[MAX_HASH];
   unsigned long long key;

#ifdef HAVE_MYDB
   if(argc < 6){
      fprintf(stderr, "usage: %s <HAM> <SPAM> <nham> <nspam> <mydb file>\n", argv[0]);
      return 1;
   }

   nham = atol(argv[3]);
   nspam = atol(argv[4]);
   f = argv[5];
#endif

   if(argc < 3){
      fprintf(stderr, "usage: %s <HAM> <SPAM>\n", argv[0]);
      return 1;
   }

   fham = fopen(argv[1], "r");
   if(!fham)
      __fatal("cannot open ham file");

   fspam = fopen(argv[2], "r");
   if(!fspam)
      __fatal("cannot open spam file");

   _inithash(tokens);

   while(fgets(buf, MAXBUFSIZE-1, fham)){
      trimBuffer(buf);
      if(strlen(buf) > 0){
         key = APHash(buf);
         _addnode(tokens, key, 1, 0);
      }
   }
   fclose(fham);

   while(fgets(buf, MAXBUFSIZE-1, fspam)){
      trimBuffer(buf);
      if(strlen(buf) > 0){
         key = APHash(buf);
         _addnode(tokens, key, 0, 1);
      }
   }
   fclose(fspam);

   _clearhash(tokens);

   return 0;
}
