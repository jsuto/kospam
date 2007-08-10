/*
 * bayes.h, 2007.05.16, SJ
 */

#include "parser.h"
#include "cfg.h"

double spamicity(char *spamfile, struct __config cfg);
struct _state parse_message(char *spamfile, struct __config cfg);

#ifdef HAVE_MYSQL
   #include <mysql.h>
   double bayes_file(MYSQL mysql, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   double bayes_file(sqlite3 *db, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
#endif

#ifdef HAVE_CDB
   double bayes_file(char *cdbfile, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
#endif

int init_cdbs(struct __config cfg);
void close_cdbs();

