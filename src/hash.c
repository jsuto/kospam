/*
 * hash.c, 2009.06.01, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hash.h"
#include "misc.h"
#include "score.h"
#include "config.h"


/*
 * reset hash
 */

void inithash(struct node *xhash[]){
   int i;

   for(i=0;i<MAXHASH;i++)
      xhash[i] = NULL;
}


/*
 * release everything in the hash and calculate the ratio of unique tokens
 */

void clearhash(struct node *xhash[], int print){
   int i;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

         if(print == 1) printf("%s (%llu) = %.4f\n", p->str, p->key, p->spaminess);
         if(print == 2) printf("%s\n", p->str);

         q = q->r;
         if(p)
            free(p);
      }
      xhash[i] = NULL;
   }
}


void printhash(struct node *xhash[]){
   int i;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

         printf("%s (%llu) = %.4f\n", p->str, p->key, p->spaminess);

         q = q->r;
      }
   }

   printf("\n\n");
}


/*
 * count nodes in hash
 */

int counthash(struct node *xhash[]){
   int i, n=0;
   struct node *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         q = q->r;
         n++;
      }
   }

   return n;
}


/*
 * create a new node
 */

struct node *makenewnode(struct node *xhash[], char *s, double spaminess, double deviation){
   struct node *h;
   int len;

   if(s == NULL)
      return NULL;

   len = strlen(s);

   if(len > MAX_TOKEN_LEN-1)
      return NULL;

   if((h = malloc(sizeof(struct node))) == NULL)
      return NULL;

   memset(h, 0, sizeof(struct node));

   strncpy(h->str, s, len);
   h->key = APHash(s);
   h->spaminess = spaminess;
   h->deviation = deviation;
   h->nham = 0;
   h->nspam = 0;
   h->num = 1;
   h->r = NULL;

   if(strchr(s, '*') || strchr(s, '+')) h->type = 1;
   else h->type = 0;

   return h;
}


/*
 * add a new node
 */

int addnode(struct node *xhash[], char *s, double spaminess, double deviation){
   struct node *p=NULL, *q;
   unsigned long long key = 0;

   if(s == NULL)
      return 0;

   key = APHash(s);

   if(xhash[hash(key)] == NULL){
      xhash[hash(key)] = makenewnode(xhash, s, spaminess, deviation);
   }
   else {
      q = xhash[hash(key)];
      while(q != NULL){
         p = q;
         if(p->key == key){
            p->num++;
            return 0;
         }
         else
            q = q->r;
      }
      p->r = makenewnode(xhash, s, spaminess, deviation);
   }

   return 1;
}


/*
 * find the given node
 */

struct node *findnode(struct node *xhash[], char *s){
   struct node *p, *q;
   unsigned long long key;

   if(s == NULL)
      return 0;

   key = APHash(s);

   q = xhash[hash(key)];

   if(q == NULL)
      return NULL;

   while(q != NULL){
      p = q;
      if(strcmp(q->str, s) == 0){
         p->num++;
         return q;
      }
      else
         q = q->r;
   }

   return NULL;
}


/*
 * update token counters
 */

int updatenode(struct node *xhash[], unsigned long long key, float nham, float nspam, float spaminess, float deviation){
   struct node *p, *q;

   q = xhash[hash(key)];

   if(q == NULL) return 0;

   while(q != NULL){
      p = q;
      if(q->key == key){
         q->nham += nham;
         q->nspam += nspam;

         if(spaminess != DEFAULT_SPAMICITY){ q->spaminess = spaminess; q->deviation = deviation; }

         return 1;
      }
      else
         q = q->r;
   }

   return 0;
}


/*
 * calculate token probabilities
 */

void calcnode(struct node *xhash[], float Nham, float Nspam, struct __config *cfg){
   int i;
   struct node *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         if(q->nham >= 0 && q->nspam >= 0 && (q->nham + q->nspam) > 0){
            q->spaminess = getTokenSpamicity(Nham, Nspam, q->nham, q->nspam, cfg->rob_s, cfg->rob_x);
            q->deviation = DEVIATION(q->spaminess);
         }

         q = q->r;
      }
   }
}


/*
 * calculate hash value
 */

inline int hash(unsigned long long key){
   return key % MAXHASH;
}


/*
 * add the tokens of a node to another
 */

int roll_tokens(struct node *uhash[], struct node *xhash[]){
   int i, n=0;
   struct node *p, *q;

   if(counthash(xhash) <= 0) return 0;

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

         if(p->spaminess != DEFAULT_SPAMICITY){
            addnode(uhash, p->str, 0.99, 0.49);
            n++;
         }

         q = q->r;
      }
   }


   return n;
}

