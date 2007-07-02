/*
 * bayes.h, 2007.05.16, SJ
 */

#include "parser.h"
#include "cfg.h"

double spamicity(char *spamfile, struct __config cfg);

double bayes_file(char *spamfile, struct session_data sdata, struct __config cfg);

int init_cdbs(struct __config cfg);
void close_cdbs();

