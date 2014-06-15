/*
 * score.h, SJ
 */

#ifndef _SCORE_H
 #define _SCORE_H

typedef struct {
   double mant;
   int exp;
} FLOAT;

float getTokenSpamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x);
double getSpamProbabilityChi2(struct node *xhash[], int *deviating_tokens, struct __config *cfg);
double applyPostSpaminessFixes(double spaminess, int found_on_rbl, int surbl_match, int has_embed_image, long c_shit, long l_shit, struct __config *cfg);

#endif /* _SCORE_H */
