/*
 * bayes.h, 2009.04.14, SJ
 */

#ifndef _BAYES_H
 #define _BAYES_H

#include "parser.h"
#include "cfg.h"
#include "hash.h"


void add_penalties(struct session_data *sdata, struct _state *state, struct __config *cfg);
void check_lists(struct session_data *sdata, struct _state *state, int *found_on_rbl, int *surbl_matc, struct __config *cfg);
float bayes_file(struct session_data *sdata, struct _state *state, struct __config *cfg);
int trainMessage(struct session_data *sdata, struct _state *state, int rounds, int is_spam, int train_mode, struct __config *cfg);

#endif /* _BAYES_H */
