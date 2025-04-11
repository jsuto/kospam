/*
 * history.c, SJ
 */

#include <kospam.h>


void write_history_to_sql(MYSQL *conn, struct session_data *sdata, struct parser_state *state){
   struct query sql;

   if(prepare_sql_statement(conn, &sql, SQL_PREPARED_STMT_INSERT_INTO_HISTORY) == ERR) return;

   p_bind_init(&sql);

   char subject[TINYBUFSIZE];
   snprintf(subject, sizeof(subject)-1, "%s", state->b_subject);

   time_t now = time(NULL);

   sql.sql[sql.pos] = (char *)&now; sql.type[sql.pos] = TYPE_LONG; sql.pos++;
   sql.sql[sql.pos] = sdata->ttmpfile; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = (char *)&(sdata->status); sql.type[sql.pos] = TYPE_LONG; sql.pos++;
   sql.sql[sql.pos] = state->envelope_from; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = &subject[0]; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = (char *)&(sdata->tot_len); sql.type[sql.pos] = TYPE_LONG; sql.pos++;

   if(p_exec_stmt(conn, &sql) == ERR) {
      syslog(LOG_PRIORITY, "%s: ERROR: insert to history", sdata->ttmpfile);
   }

   close_prepared_statement(&sql);
}
