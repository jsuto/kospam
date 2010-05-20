/*
 * session.h, SJ
 */

#include "defs.h"
#include "bayes.h"

void postfix_to_clapf(int new_sd, struct __data *data, struct __config *cfg);
int inject_mail(struct session_data *sdata, int msg, char *smtpaddr, int smtpport, char *spaminessbuf, char *buf, struct __config *cfg, char *notify);
void initSessionData(struct session_data *sdata);
void killChild();

