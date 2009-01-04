/*
 * mydb.c, 2009.01.03, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "turbo.h"
#include "buffer.h"
#include "config.h"



void initturbo(struct turbo *xhash[]){
   int i;

   for(i=0;i<MAX_TURBO_HASH;i++)
      xhash[i] = NULL;
}


void clearturbo(struct turbo *xhash[]){
   int i;
   struct turbo *p, *q;

   for(i=0;i<MAX_TURBO_HASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;
         q = q->r;
         if(p)
            free(p);
      }
      xhash[i] = NULL;
   }
}


inline unsigned long hashturbo(unsigned long long key){
   return key % MAX_TURBO_HASH;
}


struct turbo *newturbo(struct turbo *xhash[], unsigned long long key, float spaminess){
   struct turbo *h;

   if((h = malloc(sizeof(struct turbo))) == NULL)
      return NULL;

   memset(h, 0, sizeof(struct turbo));

   h->key = key;
   h->spaminess = spaminess;
   h->dirty = 0;
   h->r = NULL;

   return h;
}


int addturbo(struct turbo *xhash[], unsigned long long key, float spaminess){
   struct turbo *p=NULL, *q;

   if(xhash[hashturbo(key)] == NULL){
      xhash[hashturbo(key)] = newturbo(xhash, key, spaminess);
   }
   else {
      q = xhash[hashturbo(key)];
      while(q != NULL){
         p = q;
         if(q->key == key)
            return 0;
         else
            q = q->r;
      }
      p->r = newturbo(xhash, key, spaminess);
   }

   return 1;
}


float spamturbo(struct turbo *xhash[], unsigned long long key){
   struct turbo *p, *q;

   if(key == 0)
      return DEFAULT_SPAMICITY;

   q = xhash[hashturbo(key)];

   if(q == NULL)
      return DEFAULT_SPAMICITY;

   while(q != NULL){
      p = q;
      if(q->key == key){
         q->dirty = 1;
         return q->spaminess;
      }
      else
         q = q->r;
   }

   return DEFAULT_SPAMICITY;
}


int flush_dirty_turbo(struct turbo *xhash[], buffer *query){
   int i, n=0;
   char buf[SMALLBUFSIZE];
   struct turbo *p, *q;

   if(!query) return n;

   for(i=0;i<MAX_TURBO_HASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;
         q = q->r;
         if(p->dirty == 1){
            n++;
            snprintf(buf, SMALLBUFSIZE-1, "%llu, ", p->key);
            p->dirty = 0;
            buffer_cat(query, buf);

            if(n > MAX_TOKENS_TO_UPDATE_IN_1_ROUND) return n;
         }
      }
   }

   return n;
}

