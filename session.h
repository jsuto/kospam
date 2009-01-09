/*
 * session.h, 2009.01.09, SJ
 */

#include "bayes.h"

void init_child();

#ifdef HAVE_LIBCLAMAV
   #include <clamav.h>

   void postfix_to_clapf(int new_sd, struct __config cfg, struct cl_limits limits, struct cl_engine *engine);
#else
   void postfix_to_clapf(int new_sd, struct __config cfg);
#endif

int inject_mail(struct session_data *sdata, int msg, char *smtpaddr, int smtpport, char *spaminessbuf, struct __config *cfg, char *notify);

