/*
 * cache.c, 2009.01.04, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "buffer.h"
#include "config.h"



void initcache(struct cache *xhash[]){
   int i;

   for(i=0;i<MAX_CACHE_HASH;i++)
      xhash[i] = NULL;
}


void clearcache(struct cache *xhash[]){
   int i;
   struct cache *p, *q;

   for(i=0;i<MAX_CACHE_HASH;i++){
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


inline unsigned long hashcache(unsigned long long key){
   return key % MAX_CACHE_HASH;
}


struct cache *newcache(struct cache *xhash[], unsigned long long key, float spaminess){
   struct cache *h;

   if((h = malloc(sizeof(struct cache))) == NULL)
      return NULL;

   memset(h, 0, sizeof(struct cache));

   h->key = key;
   h->spaminess = spaminess;
   h->dirty = 0;
   h->r = NULL;

   return h;
}


int addcache(struct cache *xhash[], unsigned long long key, float spaminess){
   struct cache *p=NULL, *q;

   if(xhash[hashcache(key)] == NULL){
      xhash[hashcache(key)] = newcache(xhash, key, spaminess);
   }
   else {
      q = xhash[hashcache(key)];
      while(q != NULL){
         p = q;
         if(q->key == key)
            return 0;
         else
            q = q->r;
      }
      p->r = newcache(xhash, key, spaminess);
   }

   return 1;
}


float spamcache(struct cache *xhash[], unsigned long long key){
   struct cache *p, *q;

   if(key == 0)
      return DEFAULT_SPAMICITY;

   q = xhash[hashcache(key)];

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


int flush_dirty_cache(struct cache *xhash[], buffer *query){
   int i, n=0;
   char buf[SMALLBUFSIZE];
   struct cache *p, *q;

   if(!query) return n;

   for(i=0;i<MAX_CACHE_HASH;i++){
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


