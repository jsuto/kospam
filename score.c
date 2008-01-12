/*
 * score.c, 2008.01.12, SJ
 */

#include <stdio.h>
#include "config.h"

/*
 * calculate the spamicity of the given token
 */

float calc_spamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x, int freq_min, int w, int calc_method){
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


