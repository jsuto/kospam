/*
 * session.h, 2006.02.14, SJ
 */

#include "bayes.h"
//#include "smtp.h"

void init_child();

#ifdef HAVE_LIBCLAMAV
   #include <clamav.h>

   void postfix_to_clapf(int new_sd, struct __config cfg, struct cl_limits limits, struct cl_engine *engine);
#else
   void postfix_to_clapf(int new_sd, struct __config cfg);
#endif

