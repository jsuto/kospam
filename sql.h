/*
 * sql.h, 2007.10.09, SJ
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
   int do_mysql_qry(MYSQL mysql, int sockfd, int ham_or_spam, char *token, char *tokentable, unsigned int uid, int train_mode);
   struct te myqry(MYSQL mysql, int sockfd, char *tokentable, char *token, unsigned int uid);
   struct ue get_user_from_email(MYSQL mysql, char *email);
   int update_training_metadata(MYSQL mysql, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
#endif

#ifdef HAVE_SQLITE3
   struct te sqlite3_qry(sqlite3 *db, int sockfd, char *tokentable, char *token, unsigned int uid);
   struct ue get_user_from_email(sqlite3 *db, char *email);
   int update_training_metadata(sqlite3 *db, char *tmpfile, unsigned long uid, struct __config cfg, int is_spam);
#endif
