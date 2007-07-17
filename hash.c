/*
 * hash.c, 2007.02.26, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hash.h"
#include "misc.h"
#include "chi.h"
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

float clearhash(struct node *xhash[MAXHASH]){
   int i;
   struct node *p, *q;
   float num_of_tokens=0, num_of_unique_tokens=0;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

         num_of_tokens++;
         if(p->num == 1)
            num_of_unique_tokens++;

         q = q->r;
         if(p)
            free(p);
      }
      xhash[i] = NULL;
   }

   return (num_of_unique_tokens / num_of_tokens);
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
 * count the most interesting tokens
 */

int most_interesting_tokens(struct node *xhash[MAXHASH]){
   int i, most_interesting=0;
   struct node *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         if(DEVIATION(q->spaminess) >= MOST_INTERESTING_DEVIATION)
            most_interesting++;

         q = q->r;
      }
   }

   return most_interesting;
}

/*
 * reverse sort the hash and calculate the spamicity value
 */

double sorthash(struct node *xhash[MAXHASH], int top10, struct __config cfg){
   int i, j, n_tokens, most_interesting, how_many_tokens_to_include=top10, l=0;
   struct node *p, *q, *t[MAX_NUM_OF_SAMPLES];
   double P, Q, H, S, I, truespam=0;

   n_tokens = 0;
   most_interesting = 0;

   for(i=0; i<MAX_NUM_OF_SAMPLES; i++)
      t[i] = NULL;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL && n_tokens < MAX_NUM_OF_SAMPLES){
         t[n_tokens] = q;

         if(DEVIATION(t[n_tokens]->spaminess) >= MOST_INTERESTING_DEVIATION)
            most_interesting++;

         p = q;
         q = q->r;

         n_tokens++;
      }
   }


   /* reverse sort tokens according to their deviation */

   for(i=1; i<n_tokens; i++){
      q = t[i];
      j = i - 1;

      while(j >= 0 && q->deviation < t[j]->deviation){
         t[j+1] = t[j];
         j--;
      }

      t[j+1] = q;
   }



   /* determine how many tokens to use in the calculation */


   /* if we have too few tokens */

   if(how_many_tokens_to_include > n_tokens)
      how_many_tokens_to_include = n_tokens;

   /* if we want to include all the interesting tokens */

   if(cfg.use_all_the_most_interesting_tokens == 1 && most_interesting > top10)
      how_many_tokens_to_include = most_interesting;


   P = 1;
   Q = 1;

   //for(i=n_tokens-1; i>=0; i--){
   for(i=n_tokens-1; i>= n_tokens - how_many_tokens_to_include; i--){
      if(t[i] != NULL){

         if(t[i]->spaminess > 0.99)
            truespam++;

         /* to prevent underflow, 2006.02.22, SJ */

         if(i >= n_tokens - how_many_tokens_to_include && log10(P) > -300 && log10(Q) > -300){
            Q *= t[i]->spaminess;
            P *= 1 - t[i]->spaminess;
            l++;

         #ifdef DEBUG
            fprintf(stderr, "*");
         #endif
         }

      #ifdef DEBUG
            #ifdef HAVE_NO_64_HASH
         fprintf(stderr, "%d. %s %.4f %ld\n", i, t[i]->str, t[i]->spaminess, t[i]->num);
         #else
            fprintf(stderr, "%d. %s (%llu) %.4f %ld\n", i, t[i]->str, APHash(t[i]->str), t[i]->spaminess, t[i]->num);
         #endif
      #endif
      }
   }

#ifdef DEBUG
   fprintf(stderr, "number of tokens: %d, most interesting tokens: %d, used: %d\n", n_tokens, most_interesting, l);
   fprintf(stderr, "top10: %d, truespam: %.0f, truespam/top10: %f\n", how_many_tokens_to_include, truespam, truespam/how_many_tokens_to_include);
#endif

   /* inverse chi-square distribution with ESF values, 2006.02.24, SJ */

   Q = pow(Q, cfg.esf_h);
   P = pow(P, cfg.esf_s);


#ifdef HAVE_GSL
   H = gsl_chi2inv(-2.0 * log(Q), 2.0 * (float)l * cfg.esf_h);
   S = gsl_chi2inv(-2.0 * log(P), 2.0 * (float)l * cfg.esf_s);
#else
   /*
    * using a brand new inverse chi square implementation, 2007.02.26, SJ
    */
   H = chi2inv(-2.0 * log(Q), 2.0 * (float)l, cfg.esf_h);
   S = chi2inv(-2.0 * log(P), 2.0 * (float)l, cfg.esf_h);
#endif


   I = (1 + H - S) / 2.0;

#ifdef DEBUG
   fprintf(stderr, "with esf_h: %f, esf_s: %f\n", cfg.esf_h, cfg.esf_s);
#endif

   /* if we have a lot spammy tokens in the top10 mark the message as spam, 2005.12.09, SJ */

   /*if(I < cfg.spam_overall_limit && cfg.spam_ratio_in_top10 > 0 && truespam/how_many_tokens_to_include > cfg.spam_ratio_in_top10){
   #ifdef DEBUG
      fprintf(stderr, "original result: %f\n", I);
   #endif

      return cfg.spaminess_of_too_much_spam_in_top15;
   }*/

   return I;
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

