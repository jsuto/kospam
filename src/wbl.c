/*
 * wbl.c, SJ
 */

#include <kospam.h>


int check_email_against_list(MYSQL *conn, char *table, char *email) {
   int count = 0;
   char s[SMALLBUFSIZE];
   struct query sql;

   snprintf(s, sizeof(s)-1, "SELECT count(*) FROM %s WHERE email=?", table);

   if(prepare_sql_statement(conn, &sql, s) == ERR) return count;

   p_bind_init(&sql);
   sql.sql[sql.pos] = email; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

   if(p_exec_stmt(conn, &sql) == ERR) goto CLOSE;

   p_bind_init(&sql);
   sql.sql[sql.pos] = (char *)&count; sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;

   p_store_results(&sql);
   p_fetch_results(&sql);
   p_free_results(&sql);

CLOSE:
   close_prepared_statement(&sql);

   return count;
}
