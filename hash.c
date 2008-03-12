/*
 * hash.c, 2008.03.12, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hash.h"
#include "misc.h"
#include "config.h"


/*
 * reset hash
 */

void inithash(struct node *xhash[MAXHASH]){
   int i;

   for(i=0;i<MAXHASH;i++)
      xhash[i] = NULL;
}


/*
 * release everything in the hash and calculate the ratio of unique tokens
 */

void clearhash(struct node *xhash[MAXHASH]){
   int i;
   struct node *p, *q;

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

struct node *makenewnode(struct node *xhash[MAXHASH], char *s, double spaminess, double deviation){
   struct node *h;
   int len;

   if(s == NULL)
      return NULL;

   len = strlen(s);

   if(len > MAX_HASH_STR_LEN-1)
      return NULL;

   if((h = malloc(sizeof(struct node))) == NULL)
      return NULL;

   memset(h, 0, sizeof(struct node));

   strncpy(h->str, s, len);
   h->spaminess = spaminess;
   h->deviation = deviation;
   h->num = 1;
   h->r = NULL;

   return h;
}


/*
 * add a new node
 */

int addnode(struct node *xhash[MAXHASH], char *s, double spaminess, double deviation){
   struct node *p=NULL, *q;

   if(s == NULL)
      return 0;

   if(xhash[hash(s)] == NULL){
      xhash[hash(s)] = makenewnode(xhash, s, spaminess, deviation);
   }
   else {
      q = xhash[hash(s)];
      while(q != NULL){
         p = q;
         if(strcmp(q->str, s) == 0){
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

struct node *findnode(struct node *xhash[MAXHASH], char *s){
   struct node *p, *q;

   if(s == NULL)
      return 0;

   q = xhash[hash(s)];

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
 * calculate hash value
 */

unsigned long hash(char *s){
    unsigned long h;

    if(s == NULL)
       return 0;

    for(h=0; *s != '\0'; s++)
        h = *s + 31 * h;

    return h % MAXHASH;
}

