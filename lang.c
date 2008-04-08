/*
 * lang.c, 2008.03.18, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "parser.h"
#include "hash.h"
#include "lang.h"


int get_distance(char *s, int d, unsigned long long xxx[MAX_NGRAM]){
   int i;
   unsigned long long hash = APHash(s);

   for(i=0; i<MAX_NGRAM; i++){
      if(xxx[i] == hash) return abs(d-i);
   }

   return MAX_NGRAM;
}


int parse_ngrams(struct node *xhash[MAXHASH], char *s, int w){
   int i, j, n=0, len;
   char ngram[6];

   if(w > 5) return 0;

   len = strlen(s);

   for(i=0; i<len; i++){
      memset(ngram, 0, 6);
      memcpy(ngram, s+i, w);
      n++;

      for(j=strlen(ngram); j<w; j++)
         ngram[strlen(ngram)] = '_';

      addnode(xhash, ngram, 1, 1);
   }

   return 1;
}


unsigned long int get_distance_text(struct node *t[MAX_NUM_OF_SAMPLES], int n, unsigned long long zzz[MAX_NGRAM]){
   int i, d, delta=0;

   if(n > MAX_NGRAM) n = MAX_NGRAM;

   for(i=0; i<n; i++){
      d = get_distance(t[i]->str, i, zzz);
      delta += d;

      //printf("xx: %s %ld delta: %d\n", t[i]->str, t[i]->num, d);
   }

   return delta;
}


char *check_lang(struct _token *T){
   struct _token *p;
   struct node *s_lang[MAXHASH], *t[MAX_NUM_OF_SAMPLES], *q;
   int i, j, n, r;
   unsigned long int l_max=MAX_NGRAM*MAX_NGRAM, lang_detected_num[NUM_LANG];

   if(T == NULL) return "language detection failed";

   for(i=0; i<NUM_LANG; i++)
      lang_detected_num[i] = MAX_NGRAM*MAX_NGRAM;


   inithash(s_lang);
   p = T;
   n = 0;

   while(p){
      if(!strchr(p->str, '+') && !strchr(p->str, '*')){
         n += parse_ngrams(s_lang, p->str, 1);
         n += parse_ngrams(s_lang, p->str, 2);
         n += parse_ngrams(s_lang, p->str, 3);
         n += parse_ngrams(s_lang, p->str, 4);
         n += parse_ngrams(s_lang, p->str, 5);
      }

      p = p->r;
   }

   n = 0;

   for(i=0;i<MAXHASH;i++){
      q = s_lang[i];
      while(q != NULL && n < MAX_NUM_OF_SAMPLES){
         t[n] = q;

         q = q->r;
         n++;
      }
   }


   for(i=1; i<n; i++){
      q = t[i];
      j = i - 1;

      while(j >= 0 && q->num > t[j]->num){
         t[j+1] = t[j];
         j--;
      }

      t[j+1] = q;
   }

   /* check languages */

   lang_detected_num[0] = get_distance_text(t, n, lang_hu1);
   lang_detected_num[2] = get_distance_text(t, n, lang_en);
   //lang_detected_num[3] = get_distance_text(t, n, lang_de);
   lang_detected_num[4] = get_distance_text(t, n, lang_junk);

   r = -1;

   for(i=0; i<NUM_LANG; i++){
      if(l_max > lang_detected_num[i]){
         r = i;
         l_max = lang_detected_num[i];
      }
   }

   clearhash(s_lang);

   if(r >= 0)
      return lang_detected[r];
   else
      return "aaaa";
}


