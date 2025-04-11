/*
 * sql.h, SJ
 */

#ifndef _SQL_H
 #define _SQL_H

#include <cfg.h>
#include <defs.h>
#include <kospam.h>

#define MAX_SQL_VARS 20

struct query {
   MYSQL_STMT *stmt;
   char *sql[MAX_SQL_VARS];
   int type[MAX_SQL_VARS];
   int len[MAX_SQL_VARS];
   unsigned long length[MAX_SQL_VARS];
   my_bool is_null[MAX_SQL_VARS];
   my_bool error[MAX_SQL_VARS];
   int pos;
};

MYSQL *open_database(struct config *cfg);
void close_database(MYSQL *conn);
int select_db(MYSQL *conn, const char *db);
int prepare_sql_statement(MYSQL *conn, struct query *sql, char *s);
void p_query(MYSQL *conn, char *s);
int p_exec_stmt(MYSQL *conn, struct query *sql);
int p_store_results(struct query *sql);
int p_fetch_results(struct query *sql);
void p_free_results(struct query *sql);
void p_bind_init(struct query *sql);
uint64 p_get_insert_id(struct query *sql);
int p_get_affected_rows(struct query *sql);
void close_prepared_statement(struct query *sql);
struct te get_ham_spam_counters(MYSQL *conn, char *stmt);

#endif /* _SQL_H */
