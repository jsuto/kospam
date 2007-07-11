/*
 * qcache.c, 2007.07.08, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qcache.h"
#include "config.h"

/*
 * reset hash
 */

void inithash(struct qcache *xhash[MAXHASH]){
   int i;

   for(i=0;i<MAXHASH;i++)
      xhash[i] = NULL;
}


/*
 * release everything in the hash
 */

void clearhash(struct qcache *xhash[MAXHASH]){
   int i;
   struct qcache *p, *q;

   for(i=0;i<MAXHASH;i++){
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


/*
 * create a new node
 */

struct qcache *makenewnode(struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid, unsigned int nham, unsigned int nspam, unsigned long timestamp){
   struct qcache *h;

   if((h = malloc(sizeof(struct qcache))) == NULL)
      return NULL;

   memset(h, 0, sizeof(struct qcache));

   h->token = token;
   h->uid = uid;
   h->nham = nham;
   h->nspam = nspam;
   h->timestamp = timestamp;
   h->r = NULL;

   return h;
}


/*
 * add a new node to the list
 */

int addnode(struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid, unsigned int nham, unsigned int nspam, unsigned long timestamp){
   struct qcache *p=NULL, *q, *next=NULL;
   unsigned long ts = 0;
   int i=0;

   if(xhash[hash(token)] == NULL){
      xhash[hash(token)] = makenewnode(xhash, token, uid, nham, nspam, timestamp);
   }
   else {
      q = xhash[hash(token)];
      while(q != NULL){
         p = q;
         i++;

         if(q->timestamp > ts){
            ts = q->timestamp;
            next = q;
         }

         if(q->token == token && q->uid == uid){
            return 0;
         }
         else
            q = q->r;
      }

      /* create new entry or replace the oldest one */

      if(i < MAX_ENTRIES_PER_SLOT)
         p->r = makenewnode(xhash, token, uid, nham, nspam, timestamp);
      else {
         next->token = token;
         next->uid = uid;
         next->nham = nham;
         next->nspam = nspam;
         next->timestamp = timestamp;
         next->dirty = 0;
      }

   }

   return 1;
}


/*
 * find a specific token
 */

struct qcache *findnode(struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid){
   struct qcache *q;

   q = xhash[hash(token)];

   if(q == NULL)
      return NULL;

   while(q != NULL){
      if(q->token == token && q->uid == uid)
         return q;
      else
         q = q->r;
   }

   return NULL;
}



/*
 * calculate hash value
 */

unsigned long hash(unsigned long long token){
    return token % MAXHASH;
}

