/*
 * score.h, 2008.09.17, SJ
 */

#ifndef _SCORE_H
 #define _SCORE_H

#include "cfg.h"

float calc_spamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x);
double calc_score_chi2(struct node *xhash[], struct __config *cfg);
double apply_fixes(double spaminess, int found_on_rbl, int surbl_match, int has_embed_image, int base64_text, long c_shit, long l_shit, struct __config *cfg);

#endif /* _SCORE_H */
