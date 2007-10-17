/*
 * qcache.c, 2007.10.17, SJ
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
   struct qcache *p=NULL, *q;
   int i=0;

   if(xhash[hash(token)] == NULL){
      xhash[hash(token)] = makenewnode(xhash, token, uid, nham, nspam, timestamp);
   }
   else {
      q = xhash[hash(token)];
      while(q != NULL){
         p = q;
         i++;

         if(q->token == token && q->uid == uid){
            return 0;
         }
         else
            q = q->r;
      }

      /* append new item at the end of the list */

      p->r = makenewnode(xhash, token, uid, nham, nspam, timestamp);

      /* remove the first (and) oldest item */

      if(i >= MAX_ENTRIES_PER_SLOT){
         q = xhash[hash(token)];
         xhash[hash(token)] = q->r;
         free(q);
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

