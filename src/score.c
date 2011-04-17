/*
 * score.c, SJ
 */

#include <stdio.h>
#include <math.h>
#include <clapf.h>


float getTokenSpamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x){
   float spamicity=DEFAULT_SPAMICITY;
   int n;

   n = nham + nspam;
   if(n == 0) return DEFAULT_SPAMICITY;

   spamicity = nspam * NHAM / (nspam * NHAM + nham * NSPAM);
   spamicity = (rob_s * rob_x + n * spamicity) / (rob_s + n);

   if(spamicity < REAL_HAM_TOKEN_PROBABILITY) spamicity = REAL_HAM_TOKEN_PROBABILITY;
   if(spamicity > REAL_SPAM_TOKEN_PROBABILITY) spamicity = REAL_SPAM_TOKEN_PROBABILITY;

   return spamicity;
}


double getSpamProbabilityChi2(struct node *xhash[], struct __config *cfg){
   int i, n_tokens=0;
   struct node *p, *q;
   double H, S, I, ln2, ln_q, ln_p;
   FLOAT P = {1.0, 0}, Q = {1.0, 0};
   int e;

   /*
    * code copied from the bogofilter project
    */

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){

         if(DEVIATION(q->spaminess) >= cfg->exclusion_radius){

            /*
             * spaminess:     P = (1-p1) * (1-p2) * .... * (1-pn)
             * non-spaminess: Q = p1 * p2 * .... *pn
             */

            n_tokens++;

            Q.mant *= q->spaminess;
            if(Q.mant < 1.0e-200) {
               Q.mant = frexp(Q.mant, &e);
               Q.exp += e;
            }

            P.mant *= 1 - q->spaminess;
            if(P.mant < 1.0e-200) {
               P.mant = frexp(P.mant, &e);
               P.exp += e;
            }

            if(cfg->debug == 1) printf("%s (%llu) %.4f %ld\n", q->str, q->key, q->spaminess, q->num);
         }

         p = q;
         q = q->r;

      }
   }

   ln2 = log(2.0);
   ln_q = (log(Q.mant) + Q.exp * ln2) * cfg->esf_h;
   ln_p = (log(P.mant) + P.exp * ln2) * cfg->esf_s;

#ifdef HAVE_GSL
   H = gsl_chi2inv(-2.0 * ln_q, 2.0 * (float)n_tokens * cfg->esf_h);
   S = gsl_chi2inv(-2.0 * ln_p, 2.0 * (float)n_tokens * cfg->esf_s);
#else
   H = chi2inv(-2.0 * ln_q, 2.0 * (float)n_tokens, cfg->esf_h);
   S = chi2inv(-2.0 * ln_p, 2.0 * (float)n_tokens, cfg->esf_s);
#endif
   if(cfg->debug == 1) printf("spam=%f, ham=%f, esf_h: %f, esf_s: %f\n", H, S, cfg->esf_h, cfg->esf_s);

   I = (1 + H - S) / 2.0;

   return I;
}


double applyPostSpaminessFixes(double spaminess, int found_on_rbl, int surbl_match, int has_embed_image, long c_shit, long l_shit, struct __config *cfg){

#ifdef HAVE_RBL
   if(surbl_match > 0){
      if(cfg->debug == 1)
         printf("caught by surbl\n");

      return cfg->spaminess_of_caught_by_surbl;
   }

   if(spaminess > DEFAULT_SPAMICITY && found_on_rbl > 0){
      if(cfg->debug == 1)
         printf("caught by rbl\n");

      return cfg->spaminess_of_caught_by_rbl;
   }
#endif

   if(spaminess > DEFAULT_SPAMICITY && has_embed_image == 1) return cfg->spaminess_of_embed_image;


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

