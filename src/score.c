/*
 * score.c, 2010.05.17, SJ
 */

#include <stdio.h>
#include <math.h>
#include "hash.h"
#include "misc.h"
#include "config.h"

/*
 * calculate the spamicity of the given token
 */

float calc_spamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x){
   float r=DEFAULT_SPAMICITY;
   int n;

   n = nham + nspam;
   if(n == 0) return DEFAULT_SPAMICITY;

   /* newer calculation, and apply Robinson strength to all tokens, 2008.01.09, SJ */

   r = nspam * NHAM / (nspam * NHAM + nham * NSPAM);
   r = (rob_s * rob_x + n * r) / (rob_s + n);

   if(r < REAL_HAM_TOKEN_PROBABILITY) r = REAL_HAM_TOKEN_PROBABILITY;
   if(r > REAL_SPAM_TOKEN_PROBABILITY) r = REAL_SPAM_TOKEN_PROBABILITY;

   return r;
}


/*
 * calculate spam probability by the chi square algorithm
 */

double calc_score_chi2(struct node *xhash[], struct __config *cfg){
   int i, l=0;
   struct node *p, *q;
   double P, Q, H, S, I;

   P = 1;
   Q = 1;

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){

         if(DEVIATION(q->spaminess) >= cfg->exclusion_radius){
            if(log10(P) > -300 && log10(Q) > -300){
               Q *= q->spaminess;
               P *= 1 - q->spaminess;
               l++;

               if(cfg->debug == 1)
                  printf("%s (%llu) %.4f %ld\n", q->str, APHash(q->str), q->spaminess, q->num);
            }
         }

         p = q;
         q = q->r;

      }
   }

   Q = pow(Q, cfg->esf_h);
   P = pow(P, cfg->esf_s);

#ifdef HAVE_GSL
   H = gsl_chi2inv(-2.0 * log(Q), 2.0 * (float)l * cfg->esf_h);
   S = gsl_chi2inv(-2.0 * log(P), 2.0 * (float)l * cfg->esf_s);
#else
   /*
    * using a brand new inverse chi square implementation, 2007.02.26, SJ
    */
   H = chi2inv(-2.0 * log(Q), 2.0 * (float)l, cfg->esf_h);
   S = chi2inv(-2.0 * log(P), 2.0 * (float)l, cfg->esf_h);
#endif


   I = (1 + H - S) / 2.0;

   if(cfg->debug == 1)
      printf("with esf_h: %f, esf_s: %f\n", cfg->esf_h, cfg->esf_s);

   return I;
}


/*
 * apply some fixes
 */

double apply_fixes(double spaminess, int found_on_rbl, int surbl_match, int has_embed_image, long c_shit, long l_shit, struct __config *cfg){

   /* in case of a surbl or rbl match */
#ifdef HAVE_RBL
   if(surbl_match > 0){
      if(cfg->debug == 1)
         printf("caught by surbl\n");

      return cfg->spaminess_of_caught_by_surbl;
   }

   if(spaminess > DEFAULT_SPAMICITY && found_on_rbl > 0){
      if(cfg->debug == 1)
         printf("caught by rbl\n");

      return cfg->spaminess_of_caught_by_surbl;
   }
#endif

   /* if we shall mark the message as spam because of the embedded image */
   if(spaminess > DEFAULT_SPAMICITY && has_embed_image == 1) return cfg->spaminess_of_embed_image;


   /* check junk lines, characters */

   if(cfg->invalid_junk_limit > 0 && c_shit > cfg->invalid_junk_limit && spaminess < cfg->spam_overall_limit){
      if(cfg->debug == 1)
         printf("invalid junk characters: %ld (limit: %d)\n", c_shit, cfg->invalid_junk_limit);

      return cfg->spaminess_of_strange_language_stuff;
   }

   if(cfg->invalid_junk_line > 0 && l_shit >= cfg->invalid_junk_line && spaminess < cfg->spam_overall_limit){
      if(cfg->debug == 1)
         printf("invalid junk lines: %ld (limit: %d)\n", l_shit, cfg->invalid_junk_line);

      return cfg->spaminess_of_strange_language_stuff;
   }


   return spaminess;
}

