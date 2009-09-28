/*
 * session.h, 2009.09.27, SJ
 */

#include "defs.h"
#include "bayes.h"

void init_child();

void postfix_to_clapf(int new_sd, struct __data *data, struct __config *cfg);
int inject_mail(struct session_data *sdata, int msg, char *smtpaddr, int smtpport, char *spaminessbuf, char *buf, struct __config *cfg, char *notify);

