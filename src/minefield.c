/*
 * black.c, SJ
 */

#include <kospam.h>


void store_minefield_ip(MYSQL *conn, char *ip){
   struct query sql;

   if(prepare_sql_statement(conn, &sql, SQL_PREPARED_STMT_INSERT_INTO_MINEFIELD) == ERR) return;

   p_bind_init(&sql);

   time_t now = time(NULL);

   sql.sql[sql.pos] = ip; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = (char *)&now; sql.type[sql.pos] = TYPE_LONG; sql.pos++;

   p_exec_stmt(conn, &sql);

   close_prepared_statement(&sql);
}


bool is_sender_on_minefield(MYSQL *conn, char *ip){
   unsigned long ts=0;
   struct query sql;
   bool trapped_client = false;

   if(prepare_sql_statement(conn, &sql, SQL_PREPARED_STMT_QUERY_MINEFIELD) == ERR) return trapped_client;

   p_bind_init(&sql);

   sql.sql[sql.pos] = ip; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

   if(p_exec_stmt(conn, &sql) == ERR) goto ENDE;

   p_bind_init(&sql);

   sql.sql[sql.pos] = (char *)&ts; sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;

   p_store_results(&sql);

   if(p_fetch_results(&sql) == OK){
      trapped_client = true;
      syslog(LOG_PRIORITY, "%s is trapped on minefield", ip);
   }

   p_free_results(&sql);

ENDE:
   close_prepared_statement(&sql);

   return trapped_client;
}
