/*
 * clapf.h, 2009.01.28, SJ
 */

#include <misc.h>
#include <list.h>
#include <bayes.h>
#include <errmsg.h>
#include <messages.h>
#include <sql.h>
#include <smtpcodes.h>
#include <session.h>
#include <buffer.h>
#include <users.h>
#include <policy.h>
#include <templates.h>
#include <config.h>

#ifdef HAVE_MYSQL
   #include <mysql.h>
   int update_mysql_tokens(MYSQL mysql, struct node *xhash[], unsigned long uid);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   int update_sqlite3_tokens(sqlite3 *db, struct node *xhash[]);
#endif

#ifdef HAVE_ANTIVIRUS
#ifdef HAVE_LIBCLAMAV
   int do_av_check(struct session_data *sdata, char *email, char *email2, struct cl_limits limits, struct cl_node *root, struct __config *cfg);
#else
   int do_av_check(struct session_data *sdata, char *email, char *email2, struct __config *cfg);
#endif
#endif

void do_training(struct session_data *sdata, char *email, char *acceptbuf, struct __config *cfg);
void save_email_to_queue(struct session_data *sdata, float spaminess, struct __config *cfg);
int is_sender_on_black_or_white_list(struct session_data *sdata, char *email, char *table, struct __config *cfg);

char *check_lang(struct node *xhash[]);

