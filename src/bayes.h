/*
 * bayes.h, SJ
 */

#ifndef _BAYES_H
 #define _BAYES_H

void add_penalties(struct session_data *sdata, struct _state *state, struct __config *cfg);
float bayes_file(struct session_data *sdata, struct _state *state, struct __config *cfg);
int trainMessage(struct session_data *sdata, struct _state *state, int rounds, int is_spam, int train_mode, struct __config *cfg);

#endif /* _BAYES_H */
