/*
 * clapf.h, 2009.09.26, SJ
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
#include <hash.h>
#include <config.h>

#ifdef HAVE_MYSQL
   #include <mysql.h>
   int update_mysql_tokens(struct session_data *sdata, struct node *xhash[]);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   int update_sqlite3_tokens(struct session_data *sdata, struct node *xhash[]);
#endif

#ifdef HAVE_ANTIVIRUS
#ifdef HAVE_LIBCLAMAV
   int do_av_check(struct session_data *sdata, char *email, char *email2, char *virusinfo, struct cl_engine *engine, struct __config *cfg);
#else
   int do_av_check(struct session_data *sdata, char *email, char *email2, char *virusinfo, struct __config *cfg);
#endif
#endif

void get_queue_path(struct session_data *sdata, char **path);
void do_training(struct session_data *sdata, struct _state *state, char *email, char *acceptbuf, struct __config *cfg);
void save_email_to_queue(struct session_data *sdata, float spaminess, struct __config *cfg);
int is_sender_on_black_or_white_list(struct session_data *sdata, char *email,  char *fieldname, char *table, struct __config *cfg);

int spamc_emul(char *tmpfile, int size, struct __config *cfg);

char *check_lang(struct node *xhash[]);

int store_minefield_ip(struct session_data *sdata, struct __config *cfg);
void is_sender_on_minefield(struct session_data *sdata, char *ip, struct __config *cfg);

