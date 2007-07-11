/*
 * bayes.h, 2007.05.16, SJ
 */

#include "parser.h"
#include "cfg.h"

double spamicity(char *spamfile, struct __config cfg);

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   #include <mysql.h>
   double bayes_file(MYSQL mysql, char *spamfile, struct session_data sdata, struct __config cfg);
#else
   double bayes_file(char *spamfile, struct session_data sdata, struct __config cfg);
#endif

int init_cdbs(struct __config cfg);
void close_cdbs();

