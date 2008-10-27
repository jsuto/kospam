/*
 * sql.h, 2008.10.25, SJ
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

typedef struct {
#ifdef HAVE_MYSQL
   MYSQL mysql;
#endif
#ifdef HAVE_SQLITE3
   sqlite3 *db;
#endif
   int sockfd;
   float ham_msg;
   float spam_msg;
   unsigned long uid;
   float rob_x;
   float rob_s;
} qry;


struct te {
   unsigned int nham;
   unsigned int nspam;
};

#ifdef HAVE_MYSQL
   unsigned long get_uid(MYSQL mysql, char *stmt);
   struct te get_ham_spam(MYSQL mysql, char *stmt);
   int do_mysql_qry(MYSQL mysql, int sockfd, int ham_or_spam, char *token, unsigned long uid, int train_mode, unsigned long timestamp);
   struct te myqry(MYSQL mysql, int sockfd, char *token, unsigned long uid);
   int is_sender_on_white_list(MYSQL mysql, char *email, unsigned long uid, struct __config cfg);
   void insert_2_queue(MYSQL mysql, struct session_data *sdata, struct __config cfg, int is_spam);
   int update_training_metadata(MYSQL mysql, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
   int my_walk_hash(MYSQL mysql, int sockfd, int ham_or_spam, char *tokentable, unsigned long uid, struct _token *token, int train_mode);
#endif

#ifdef HAVE_SQLITE3
   int do_sqlite3_qry(sqlite3 *db, int ham_or_spam, char *token, int train_mode, unsigned long timestamp);
   struct te sqlite3_qry(sqlite3 *db, char *token);
   int is_sender_on_white_list(sqlite3 *db, char *email, unsigned long uid, struct __config cfg);
   void insert_2_queue(sqlite3 *db, struct session_data *sdata, struct __config cfg, int is_spam);
   int update_training_metadata(sqlite3 *db, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
   int my_walk_hash(sqlite3 *db, int ham_or_spam, char *tokentable, struct _token *token, int train_mode);
#endif

float SQL_QUERY(qry QRY, int group_type, char *tokentable, char *token);

#endif /* _SQL_H */
