/*
 * bayes.h, 2008.01.23, SJ
 */

#include "parser.h"
#include "cfg.h"

double spamicity(char *spamfile, struct __config cfg);
struct _state parse_message(char *spamfile, struct __config cfg);

#ifdef HAVE_MYSQL
   #include <mysql.h>
   struct c_res bayes_file(MYSQL mysql, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
   int train_message(MYSQL mysql, struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   struct c_res bayes_file(sqlite3 *db, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
   int train_message(sqlite3 *db, struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg);
#endif
