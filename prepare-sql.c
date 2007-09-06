/*
 * prepare-sql.c, 2007.09.05, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "messages.h"
#include "config.h"


#define MAXHASH 74713

struct node {
   unsigned long long key;
   unsigned int nham;
   unsigned int nspam;
   struct node *r;
};


void inithash(struct node *xhash[MAXHASH]){
   int i;

   for(i=0;i<MAXHASH;i++)
      xhash[i] = NULL;
}


void clearhash(struct node *xhash[MAXHASH]){
   int i;
   struct node *p, *q;

#ifdef HAVE_SQLITE3
   printf("BEGIN;\n");
#endif

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

      #ifdef HAVE_MYSQL
         printf("INSERT INTO t_token (token, uid, nham, nspam) VALUES(%llu, 0, %d, %d);\n", p->key, p->nham, p->nspam);
      #endif
      #ifdef HAVE_SQLITE3
         printf("INSERT INTO t_token (token, uid, nham, nspam) VALUES('%llu', 0, %d, %d);\n", p->key, p->nham, p->nspam);
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

}


unsigned long hash(unsigned long long key){
    return key % MAXHASH;
}

struct node *makenewnode(struct node *xhash[MAXHASH], unsigned long long key, unsigned int nham, unsigned int nspam){
   struct node *h;

   if((h = malloc(sizeof(struct node))) == NULL)
      return NULL;

   memset(h, 0, sizeof(struct node));

   h->key = key;
   h->nham = nham;
   h->nspam = nspam;
   h->r = NULL;

   return h;
}


int addnode(struct node *xhash[MAXHASH], unsigned long long key, unsigned int nham, unsigned int nspam){
   struct node *p=NULL, *q;

   if(xhash[hash(key)] == NULL){
      xhash[hash(key)] = makenewnode(xhash, key, nham, nspam);
   }
   else {
      q = xhash[hash(key)];
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
      p->r = makenewnode(xhash, key, nham, nspam);
   }

   return 1;
}


int main(int argc, char **argv){
   FILE *fham, *fspam;
   char buf[MAXBUFSIZE];
   struct node *tokens[MAXHASH];
   unsigned long long key;

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

   inithash(tokens);

   while(fgets(buf, MAXBUFSIZE-1, fham)){
      trim(buf);
      key = APHash(buf);
      addnode(tokens, key, 1, 0);
   }
   fclose(fham);

   while(fgets(buf, MAXBUFSIZE-1, fspam)){
      trim(buf);
      key = APHash(buf);
      addnode(tokens, key, 0, 1);
   }
   fclose(fspam);

   clearhash(tokens);

   return 0;
}
