/*
 * bayes.h, 2009.01.09, SJ
 */

#ifndef _BAYES_H
 #define _BAYES_H

#include "parser.h"
#include "cfg.h"
#include "hash.h"


double spamicity(char *spamfile, struct __config cfg);
struct _state parse_message(char *spamfile, struct session_data *sdata, struct __config *cfg);

void add_penalties(struct session_data *sdata, struct _state *state, struct __config *cfg);
void check_lists(struct session_data *sdata, struct _state *state, int *found_on_rbl, int *surbl_matc, struct __config *cfg);


#ifdef HAVE_MYSQL
   #include <mysql.h>
   float bayes_file(MYSQL mysql, struct _state *state, struct session_data *sdata, struct __config *cfg);
   int train_message(MYSQL mysql, struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   float bayes_file(sqlite3 *db, struct _state *state, struct session_data *sdata, struct __config *cfg);
   int train_message(sqlite3 *db, struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg);
#endif

#ifdef HAVE_MYDB
   #include "mydb.h"
   float bayes_file(struct _state *state, struct session_data *sdata, struct __config *cfg);
   int train_message(char *mydbfile, struct mydb_node *mhash[], struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg);
#endif

#endif /* _BAYES_H */
