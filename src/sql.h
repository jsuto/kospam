/*
 * sql.h, SJ
 */

#ifndef _SQL_H
 #define _SQL_H

#include <config.h>
#include <defs.h>
#include <cfg.h>
#include <kospam.h>

int open_database(struct session_data *sdata, struct __config *cfg);
void close_database(struct session_data *sdata);
int select_db(struct session_data *sdata, const char *db);
int prepare_sql_statement(struct session_data *sdata, struct sql *sql, char *s);
void p_query(struct session_data *sdata, char *s);
int p_exec_stmt(struct session_data *sdata, struct sql *sql);
int p_store_results(struct sql *sql);
int p_fetch_results(struct sql *sql);
void p_free_results(struct sql *sql);
void p_bind_init(struct sql *sql);
uint64 p_get_insert_id(struct sql *sql);
int p_get_affected_rows(struct sql *sql);
void close_prepared_statement(struct sql *sql);
struct te get_ham_spam_counters(struct session_data *sdata, char *stmt);
void update_hash(struct session_data *sdata, char *qry, struct node *xhash[]);

#endif /* _SQL_H */

