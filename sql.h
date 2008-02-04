/*
 * sql.h, 2008.01.28, SJ
 */

#ifdef HAVE_MYSQL
   #include <mysql.h>
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
#endif

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

struct ue {
   unsigned long uid;
   char name[SMALLBUFSIZE];
};

#ifdef HAVE_MYSQL
   unsigned long get_uid(MYSQL mysql, char *stmt);
   struct te get_ham_spam(MYSQL mysql, char *stmt);
   int do_mysql_qry(MYSQL mysql, int sockfd, int ham_or_spam, char *token, char *tokentable, unsigned long uid, int train_mode);
   struct te myqry(MYSQL mysql, int sockfd, char *tokentable, char *token, unsigned long uid);
   struct ue get_user_from_email(MYSQL mysql, char *email);
   int is_sender_on_white_list(MYSQL mysql, char *email, unsigned long uid);
   void insert_2_queue(MYSQL mysql, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
   int update_training_metadata(MYSQL mysql, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
#endif

#ifdef HAVE_SQLITE3
   int do_sqlite3_qry(sqlite3 *db, int ham_or_spam, char *token, int train_mode, unsigned long timestamp);
   struct te sqlite3_qry(sqlite3 *db, char *token);
   struct ue get_user_from_email(sqlite3 *db, char *email);
   int is_sender_on_white_list(sqlite3 *db, char *email, unsigned long uid);
   void insert_2_queue(sqlite3 *db, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
   int update_training_metadata(sqlite3 *db, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
#endif
