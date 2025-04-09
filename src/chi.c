/*
 * chi.c, SJ
 */

#include <kospam.h>

#ifdef HAVE_GSL

#include <gsl/gsl_cdf.h>

#define DBL_EPSILON 2.2204460492503131e-16

double gsl_chi2inv(double x, double df){
   double R;

   R = gsl_cdf_chisq_Q(x, df);

   if(R < DBL_EPSILON)
      return 0.0;
   else
      return R;
}

#else


#define MAX_ALLOWABLE_M 700.0

int iHalfDF, iAdjustedHalfDF, iAdjustedDF;
double fAdjustedProp, fAdjustedChi, fM;

double factorial(int x);
void makeAdjustments(double fChi, int iDF, double fESF);
double chi2p_few_tokens(double fChi, double iChiDF, double fESF);
double chi2p_many_tokens(double fChi, int iDF, double fESF);


/*
 * factorial of an integer number
 */

double factorial(int x){
   int i;
   double f = 1;

   for(i=1; i<=x; i++)
      f *= i;

   return f;
}


/*
 * inverse chi square function with a small degrees of freedom (< 25-30) value
 */

double chi2p_few_tokens(double fChi, double iChiDF, double fESF){
   int i;
   double fAdjustedProduct, iActualSize, fEffectiveSize, fSum;

   fAdjustedProduct = exp((fESF * (-fChi)/2.0));

   iActualSize = iChiDF / 2;
   fEffectiveSize = iActualSize * fESF;

   fSum = 0.0;

   for(i=0; i<(int)fEffectiveSize; i++)
      fSum += pow(-log(fAdjustedProduct), i) / factorial(i);


   double fFirstTerm = fAdjustedProduct * fSum;

   double fSecondTerm = fAdjustedProduct * (fEffectiveSize - (int)fEffectiveSize) * pow(-log(fAdjustedProduct), (int)fEffectiveSize) / factorial((int)fEffectiveSize);

   return fFirstTerm + fSecondTerm;
}


/*
 * adjust values
 */

void makeAdjustments(double fChi, int iDF, double fESF){
   iHalfDF = iDF / 2;

   iAdjustedHalfDF = 1;
   if(fESF * iHalfDF + .5 > 1)
      iAdjustedHalfDF = fESF * iHalfDF + .5;

   fAdjustedProp =  (float)iAdjustedHalfDF / iHalfDF;

   fAdjustedChi = fChi * fAdjustedProp;

   iAdjustedDF = iAdjustedHalfDF * 2;

   fM = fAdjustedChi / 2.0;
}


/*
 * inverse chi square function for greater degrees of freedom values
 */

double chi2p_many_tokens(double fChi, int iDF, double fESF){
   int i;
   double fSum, fTerm;

   makeAdjustments(fChi, iDF, fESF);

   if(fM > MAX_ALLOWABLE_M){
      fESF = fESF * (MAX_ALLOWABLE_M / fM);
      makeAdjustments(fChi, iDF, fESF);
   }

   fSum = fTerm = exp(-fM);

   for(i=1; i<iAdjustedDF/2; i++){
      fTerm *= fM / i;
      fSum += fTerm;
   }

   if(fSum < 1)
      return fSum;
   else
      return 1.0;
}



/*
 * inverse chi square algorithm based on http://garyrob.blogs.com/chi2p.py
 */

double chi2inv(double x, double df, double esf){
   if(df * esf > 25)
      return chi2p_many_tokens(x, df, esf);
   else
      return chi2p_few_tokens(x, df, esf);
}


#endif


double get_spam_probability(struct node *xhash[], int *deviating_tokens, struct __config *cfg){
   int i, n_tokens=0;
   struct node *q;
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

            if(cfg->debug == 1) printf("%s (%llu) %.4f %.0f/%.0f\n", (char *)q->str, q->key, q->spaminess, q->nham, q->nspam);
         }

         q = q->r;

      }
   }

   *deviating_tokens = n_tokens;

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
