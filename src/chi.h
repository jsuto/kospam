/*
 * chi.h, SJ
 */

#ifndef _CHI_H
 #define _CHI_H

double gsl_chi2inv(double x, double df);
double chi2inv(double x, double df, double esf);
double get_spam_probability(struct node *xhash[], int *deviating_tokens, struct __config *cfg);

#endif /* _CHI_H */
