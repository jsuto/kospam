/*
 * clapf.h, 2009.01.08, SJ
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

char *check_lang(struct node *xhash[]);

