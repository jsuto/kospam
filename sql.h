/*
 * sql.h, 2009.01.08, SJ
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
   struct te get_ham_spam(MYSQL mysql, char *stmt);
   int do_mysql_qry(MYSQL mysql, int ham_or_spam, char *token, unsigned long uid, int train_mode, unsigned long timestamp);
   int update_hash(MYSQL mysql, char *qry, float Nham, float Nspam, struct node *xhash[], struct __config *cfg);
   int is_sender_on_white_list(MYSQL mysql, char *email, unsigned long uid, struct __config cfg);
   void insert_2_queue(MYSQL mysql, struct session_data *sdata, struct __config cfg, int is_spam);
   int update_training_metadata(MYSQL mysql, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
   int my_walk_hash(MYSQL mysql, int ham_or_spam, unsigned long uid, struct node *xhash[], int train_mode);
#endif

#ifdef HAVE_SQLITE3
   int do_sqlite3_qry(sqlite3 *db, int ham_or_spam, char *token, int train_mode, unsigned long timestamp);
   struct te sqlite3_qry(sqlite3 *db, char *token);
   int update_hash(sqlite3 *db, char *qry, float Nham, float Nspam, struct node *xhash[], struct __config *cfg);
   int is_sender_on_white_list(sqlite3 *db, char *email, unsigned long uid, struct __config cfg);
   void insert_2_queue(sqlite3 *db, struct session_data *sdata, struct __config cfg, int is_spam);
   int update_training_metadata(sqlite3 *db, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
   int my_walk_hash(sqlite3 *db, int ham_or_spam, struct node *xhash[], int train_mode);
#endif

#endif /* _SQL_H */
