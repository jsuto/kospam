/*
 * chi.c, 2008.08.22, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef HAVE_GSL

#include <gsl/gsl_cdf.h>

#define DBL_EPSILON 2.2204460492503131e-16

double gsl_chi2inv(double x, double df){
   double R;

   R = gsl_cdf_chisq_Q(x, df);

   /*printf("x=%f, df=%f, R=%f\n", x, df, R);*/

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
double chi2pFewTokens(double fChi, double iChiDF, double fESF);
double chi2pManyTokens(double fChi, int iDF, double fESF);


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

double chi2pFewTokens(double fChi, double iChiDF, double fESF){
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

double chi2pManyTokens(double fChi, int iDF, double fESF){
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
      return chi2pManyTokens(x, df, esf);
   else
      return chi2pFewTokens(x, df, esf);
}

/*
 * inverse chi-square function, taken from
 * http://www.linuxjournal.com/articles/lj/0107/6467/6467s2.html
 *
 * update (2007.02.26): now it's obsoleted by the previous implementation
 */

double chi2inv_old(double x, int df, double esf){
   int i, v;
   double m, sum, term;

   v = df * esf;

   m = x/2;
   sum = term = exp(-m);

   for(i=1; i<(v/2); i++){
      term *= m/i;
      sum += term;
   }

   /*printf("x=%f, df=%d, R=%f\n", x, v, sum);*/

   if(sum < 1)
      return sum;
   else
      return 1.0;
}

#endif

