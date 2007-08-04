/*
 * sql.h, 2007.08.03, SJ
 */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   #include <mysql.h>
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
#endif

typedef struct {
#ifdef HAVE_MYSQL_TOKEN_DATABASE
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


#ifdef HAVE_MYSQL_TOKEN_DATABASE
   unsigned long get_uid(MYSQL mysql, char *stmt);
   struct te get_ham_spam(MYSQL mysql, char *stmt);
   int do_mysql_qry(MYSQL mysql, int sockfd, int ham_or_spam, char *token, char *tokentable, unsigned int uid, int train_mode);
   struct te myqry(MYSQL mysql, int sockfd, char *tokentable, char *token, unsigned int uid);
   unsigned long get_uid_from_email(MYSQL mysql, char *tmpfile, char *rcptto);
   int update_training_metadata(MYSQL mysql, char *tmpfile, unsigned long uid, struct __config cfg);
#endif

#ifdef HAVE_SQLITE3
   struct te sqlite3_qry(sqlite3 *db, int sockfd, char *tokentable, char *token, unsigned int uid);
   unsigned long get_uid_from_email(sqlite3 *db, char *tmpfile, char *rcptto);
   int update_training_metadata(sqlite3 *db, char *tmpfile, unsigned long uid, struct __config cfg);
#endif
