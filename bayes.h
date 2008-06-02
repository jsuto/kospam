/*
 * bayes.h, 2008.05.07, SJ
 */

#ifndef _BAYES_H
 #define _BAYES_H

#include "parser.h"
#include "cfg.h"

double spamicity(char *spamfile, struct __config cfg);
struct _state parse_message(char *spamfile, struct session_data sdata, struct __config cfg);

#ifdef HAVE_MYSQL
   #include <mysql.h>
   struct c_res bayes_file(MYSQL mysql, struct _state state, struct session_data sdata, struct __config cfg);
   int train_message(MYSQL mysql, struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   struct c_res bayes_file(sqlite3 *db, struct _state state, struct session_data sdata, struct __config cfg);
   int train_message(sqlite3 *db, struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg);
#endif

#ifdef HAVE_MYDB
   #include "mydb.h"
   struct c_res bayes_file(struct mydb_node *mhash[MAX_MYDB_HASH], struct _state state, struct session_data sdata, struct __config cfg);
   int train_message(char *mydbfile, struct mydb_node *mhash[MAX_MYDB_HASH], struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg);
#endif

#endif /* _BAYES_H */
