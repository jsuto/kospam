/*
 * sql.h, 2010.05.13, SJ
 */

#ifndef _SQL_H
 #define _SQL_H

#ifdef HAVE_MYSQL
   #include <mysql.h>
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
#endif

#include "parser.h"
#include "defs.h"
#include "cfg.h"

#ifdef HAVE_MYSQL
   unsigned long get_uid(MYSQL mysql, char *stmt);
   struct te getNumberOfHamSpamMessages(MYSQL mysql, char *stmt);
   int do_mysql_qry(MYSQL mysql, int ham_or_spam, char *token, unsigned long uid, int train_mode, unsigned long timestamp);
   int update_hash(MYSQL mysql, char *qry, struct node *xhash[], struct __config *cfg);
   int my_walk_hash(MYSQL mysql, int ham_or_spam, unsigned long uid, struct node *xhash[], int train_mode);
#endif

#ifdef HAVE_SQLITE3
   int do_sqlite3_qry(sqlite3 *db, int ham_or_spam, char *token, int train_mode, unsigned long timestamp);
   struct te sqlite3_qry(sqlite3 *db, char *token);
   int update_hash(sqlite3 *db, char *qry, struct node *xhash[], struct __config *cfg);
   int my_walk_hash(sqlite3 *db, int ham_or_spam, struct node *xhash[], int train_mode);
#endif

#endif /* _SQL_H */
