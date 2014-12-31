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


void store_minefield_ip(struct session_data *sdata, struct __data *data, char *ip, struct __config *cfg){
   if(prepare_sql_statement(sdata, &(data->stmt_insert_into_blackhole), SQL_PREPARED_STMT_INSERT_INTO_BLACKHOLE) == ERR) return;

   p_bind_init(data);

   data->sql[data->pos] = ip; data->type[data->pos] = TYPE_STRING; data->pos++;
   data->sql[data->pos] = (char *)&(sdata->now); data->type[data->pos] = TYPE_LONG; data->pos++;

   p_exec_query(sdata, data->stmt_insert_into_blackhole, data);

   mysql_stmt_close(data->stmt_insert_into_blackhole);
}


void is_sender_on_minefield(struct session_data *sdata, struct __data *data, char *ip, struct __config *cfg){
   unsigned long ts=0;

   if(prepare_sql_statement(sdata, &(data->stmt_generic), SQL_PREPARED_STMT_QUERY_MINEFIELD) == ERR) return;

   p_bind_init(data);

   data->sql[data->pos] = ip; data->type[data->pos] = TYPE_STRING; data->pos++;

   if(p_exec_query(sdata, data->stmt_generic, data) == ERR) goto ENDE;

   p_bind_init(data);

   data->sql[data->pos] = (char *)&ts; data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;

   p_store_results(sdata, data->stmt_generic, data);

   if(p_fetch_results(data->stmt_generic) == OK){
      sdata->trapped_client = 1;
      syslog(LOG_PRIORITY, "%s: %s is trapped on minefield", sdata->ttmpfile, ip);
   }

   p_free_results(data->stmt_generic);

ENDE:
   close_prepared_statement(data->stmt_generic);

}

