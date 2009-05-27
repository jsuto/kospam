/*
 * session.h, 2009.05.27, SJ
 */

#include "defs.h"
#include "bayes.h"

void init_child();

#ifdef HAVE_LIBCLAMAV
   #include <clamav.h>

   void postfix_to_clapf(int new_sd, struct url *blackhole, struct cl_engine *engine, struct __config *cfg);
#else
   void postfix_to_clapf(int new_sd, struct url *blackhole, struct __config *cfg);
#endif

int inject_mail(struct session_data *sdata, int msg, char *smtpaddr, int smtpport, char *spaminessbuf, struct __config *cfg, char *notify);

