/*
 * hash.c, SJ
 */

#include <kospam.h>


inline int hash(uint64 key){
   return key % MAXHASH;
}


void inithash(struct node *xhash[]){
   int i;

   for(i=0;i<MAXHASH;i++){
      xhash[i] = NULL;
   }
}


void clearhash(struct node *xhash[]){
   int i;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

         q = q->r;
         if(p){
            if(p->str){
               free(p->str);
            }
            free(p);
         }
      }
      xhash[i] = NULL;
   }
}


void resetcounters(struct node *xhash[]){
   int i;
   struct node *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         q->nham = q->nspam = 0;
         q = q->r;
      }
   }
}


void printhash(struct node *xhash[]){
   int i;
   struct node *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         printf("%s\n", (char *)q->str);
         q = q->r;
      }
   }
}


struct node *makenewnode(char *s, double spaminess, double deviation){
   struct node *h;
   int len;

   if(s == NULL) return NULL;

   len = strlen(s);

   if((h = malloc(sizeof(struct node))) == NULL) return NULL;

   memset(h, 0, sizeof(struct node));

   h->str = malloc(len+2);

   if(h->str == NULL){
      free(h);
      return NULL;
   }

   memset(h->str, 0, len+2);

   snprintf(h->str, len+1, "%s", s);

   h->key = xxh3_64(s);
   h->spaminess = spaminess;
   h->deviation = deviation;
   h->nham = 0;
   h->nspam = 0;

   if(strchr(s, '*') || strchr(s, '+') || strchr(s, ':') || strchr(s, '%')) h->type = 1;
   else h->type = 0;

   h->r = NULL;

   return h;
}


int addnode(struct node *xhash[], char *s, double spaminess, double deviation){
   struct node *p=NULL, *q;
   uint64 key = 0;

   if(s == NULL) return 0;

   key = xxh3_64(s);

   if(xhash[hash(key)] == NULL){
      xhash[hash(key)] = makenewnode(s, spaminess, deviation);
   }
   else {
      q = xhash[hash(key)];
      while(q != NULL){
         p = q;
         if(p->key == key){
            return 0;
         }
         else {
            q = q->r;
         }
      }
      p->r = makenewnode(s, spaminess, deviation);
   }

   return 1;
}


struct node *findnode(struct node *xhash[], char *s){
   struct node *q;
   uint64 key;

   if(s == NULL) return NULL;

   key = xxh3_64(s);

   q = xhash[hash(key)];

   if(q == NULL) return NULL;

   while(q != NULL){
      if(q->key == key){
         return q;
      }
      else {
         q = q->r;
      }
   }

   return NULL;
}


/*
 * update token counters
 */

int updatenode(struct node *xhash[], uint64 key, float nham, float nspam, float spaminess, float deviation){
   struct node *q;

   q = xhash[hash(key)];

   if(q == NULL) return 0;

   while(q != NULL){

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


int count_existing_tokens_in_token_table(struct node *xhash[]) {
    struct node *q;
    int count = 0;

    for(int i=0; i<MAXHASH; i++){
        q = xhash[i];
        while(q != NULL) {
            if (q->nham + q->nspam > 0) {
               count++;
            }
            q = q->r;
        }
    }

    return count;
}
