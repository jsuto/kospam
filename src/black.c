/*
 * black.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h>
#include <clapf.h>


void store_minefield_ip(struct session_data *sdata, char *ip){
   struct sql sql;

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_INSERT_INTO_BLACKHOLE) == ERR) return;

   p_bind_init(&sql);

   sql.sql[sql.pos] = ip; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = (char *)&(sdata->now); sql.type[sql.pos] = TYPE_LONG; sql.pos++;

   p_exec_stmt(sdata, &sql);

   close_prepared_statement(&sql);
}


void is_sender_on_minefield(struct session_data *sdata, char *ip){
   unsigned long ts=0;
   struct sql sql;

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_QUERY_MINEFIELD) == ERR) return;

   p_bind_init(&sql);

   sql.sql[sql.pos] = ip; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

   if(p_exec_stmt(sdata, &sql) == ERR) goto ENDE;

   p_bind_init(&sql);

   sql.sql[sql.pos] = (char *)&ts; sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;

   p_store_results(&sql);

   if(p_fetch_results(&sql) == OK){
      sdata->trapped_client = 1;
      syslog(LOG_PRIORITY, "%s: %s is trapped on minefield", sdata->ttmpfile, ip);
   }

   p_free_results(&sql);

ENDE:
   close_prepared_statement(&sql);

}

