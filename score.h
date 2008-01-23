/*
 * score.h, 2008.01.22, SJ
 */

float calc_spamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x);
double calc_score_chi2(struct node *xhash[MAXHASH], struct __config cfg);

