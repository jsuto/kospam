/*
 * bayes.h, 2007.10.10, SJ
 */

#include "parser.h"
#include "cfg.h"

double spamicity(char *spamfile, struct __config cfg);
struct _state parse_message(char *spamfile, struct __config cfg);
int tum_train(char *spamfile, double spaminess, struct __config cfg);

#ifdef HAVE_MYSQL
   #include <mysql.h>
   double bayes_file(MYSQL mysql, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
   int retraining(MYSQL mysql, struct session_data sdata, char *username, int is_spam, struct __config cfg);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   double bayes_file(sqlite3 *db, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
   int retraining(sqlite3 *db, struct session_data sdata, char *username, int is_spam, struct __config cfg);
#endif

#ifdef HAVE_MYDB
   double bayes_file(char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
   int retraining(struct session_data sdata, char *username, int is_spam, struct __config cfg);
#endif


