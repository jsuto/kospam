/*
 * clapf.h, 2008.02.22, SJ
 */

#include <misc.h>
#include <bayes.h>
#include <errmsg.h>
#include <messages.h>
#include <sql.h>
#include <black.h>
#include <smtpcodes.h>
#include <config.h>
#include <mydb.h>

#ifdef HAVE_MYSQL
   #include <mysql.h>
   int update_mysql_tokens(MYSQL mysql, struct _token *token, unsigned long uid);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   int update_sqlite3_tokens(sqlite3 *db, struct _token *token);
#endif

#ifdef HAVE_MYDB
   struct c_res bayes_file(struct mydb_node *mhash[MAX_MYDB_HASH], char *spamfile, struct _state state, struct session_data sdata, struct __config cfg);
   int train_message(char *mydbfile, struct mydb_node *mhash[MAX_MYDB_HASH], struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg);
#endif

