/*
 * score.h, 2010.05.19, SJ
 */

#ifndef _SCORE_H
 #define _SCORE_H

#include "cfg.h"

float getTokenSpamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x);
double getSpamProbabilityChi2(struct node *xhash[], struct __config *cfg);
double applyPostSpaminessFixes(double spaminess, int found_on_rbl, int surbl_match, int has_embed_image, long c_shit, long l_shit, struct __config *cfg);

#endif /* _SCORE_H */
