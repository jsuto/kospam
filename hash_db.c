/*
 * hash_db.c, 2006.08.25, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_db.h"
#include "config.h"

/*
 * reset hash
 */

void init_token_hash(struct t_node *xhash[MAX_TOKEN_HASH]){
   int i;

   for(i=0;i<MAX_TOKEN_HASH;i++)
      xhash[i] = NULL;
}

/*
 * release everything in the hash and calculate the ratio of unique tokens
 */

void clear_token_hash(struct t_node *xhash[MAX_TOKEN_HASH]){
   int i;
   struct t_node *p, *q;

   for(i=0;i<MAX_TOKEN_HASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;
         q = q->r;
         if(p){
            if(p->str)
               free(p->str);

            free(p);
         }
      }
      xhash[i] = NULL;
   }
}

/*
 * create a new node
 */

struct t_node *makenewtoken(struct t_node *xhash[MAX_TOKEN_HASH], char *s, float spaminess){
   struct t_node *h;
   int len;

   if(s == NULL)
      return NULL;

   len = strlen(s);

   if(len > MAX_HASH_STR_LEN-1)
      return NULL;

   if((h = malloc(sizeof(struct t_node))) == NULL)
      return NULL;

   memset(h, 0, sizeof(struct t_node));

   h->str = malloc(len+1);
   if(!h->str){
      free(h);
      return NULL;
   }

   memset(h->str, 0, len+1);
   strncpy(h->str, s, len);
   h->spaminess = spaminess;
   h->r = NULL;

   return h;
}

/*
 * add a new node
 */

int addtoken(struct t_node *xhash[MAX_TOKEN_HASH], char *s, float spaminess){
   struct t_node *p=NULL, *q;

   if(s == NULL)
      return 0;

   if(xhash[hash_token(s)] == NULL){
      xhash[hash_token(s)] = makenewtoken(xhash, s, spaminess);
   }
   else {
      q = xhash[hash_token(s)];
      while(q != NULL){
         p = q;
         if(strcmp(q->str, s) == 0)
            return 0;
         else
            q = q->r;
      }
      p->r = makenewtoken(xhash, s, spaminess);
   }

   return 1;
}

/*
 * returns the spamicity of the given node
 */

float get_spamicity(struct t_node *xhash[MAX_TOKEN_HASH], char *s){
   struct t_node *p, *q;

   if(s == NULL)
      return DEFAULT_SPAMICITY;

   q = xhash[hash_token(s)];

   if(q == NULL)
      return DEFAULT_SPAMICITY;

   while(q != NULL){
      p = q;
      if(strcmp(q->str, s) == 0){
         return q->spaminess;
      }
      else
         q = q->r;
   }

   return DEFAULT_SPAMICITY;
}

/*
 * calculate hash value
 */

unsigned long hash_token(char *s){
    unsigned long h;

    if(s == NULL)
       return 0;

    for(h=0; *s != '\0'; s++)
        h = *s + 31 * h;

    return h % MAX_TOKEN_HASH;
}

int read_datafile(char *filename, struct t_node *xhash[MAX_TOKEN_HASH]){
   FILE *f;
   char buf[SMALLBUFSIZE], *p;
   float spaminess;

   f = fopen(filename, "r");
   if(!f)
      return 0;

   while(fgets(buf, SMALLBUFSIZE-1, f)){
      p = strchr(buf, ' ');
      if(p){
         *p = '\0';
         p++;
         spaminess = atof(p);
         addtoken(xhash, buf, spaminess);
      }
   }

   return 1;
}
