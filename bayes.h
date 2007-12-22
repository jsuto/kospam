/*
 * bayes.h, 2007.12.22, SJ
 */

#include "parser.h"
#include "cfg.h"

double spamicity(char *spamfile, struct __config cfg);
struct _state parse_message(char *spamfile, struct __config cfg);

#ifdef HAVE_MYSQL
   #include <mysql.h>
   int tum_train(MYSQL mysql, char *spamfile, double spaminess, struct __config cfg);
   double bayes_file(MYSQL mysql, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
   int retraining(MYSQL mysql, struct session_data sdata, char *filename, int is_spam, struct __config cfg);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   int tum_train(sqlite3 *db, char *spamfile, double spaminess, struct __config cfg);
   double bayes_file(sqlite3 *db, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
   int retraining(sqlite3 *db, struct session_data sdata, char *filename, int is_spam, struct __config cfg);
#endif

#ifdef HAVE_MYDB
   int tum_train(char *spamfile, double spaminess, struct __config cfg);
   double bayes_file(char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
   int retraining(struct session_data sdata, char *filename, int is_spam, struct __config cfg);
#endif


