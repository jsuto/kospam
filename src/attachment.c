/*
 * attachment.c, SJ
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <clapf.h>


void update_bad_attachment_counter(struct session_data *sdata, char *digest){
   struct sql sql;

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_UPDATE_ADIGEST) == OK){

      p_bind_init(&sql);
      sql.sql[sql.pos] = digest; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

      p_exec_stmt(sdata, &sql);

      close_prepared_statement(&sql);
   }
}


int check_for_known_bad_attachments(struct session_data *sdata, struct __state *state){
   int i, counter=0, rc=AVIR_OK;
   struct sql sql;

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_QUERY_ADIGEST) == ERR) return rc;

   for(i=1; i<=state->n_attachments; i++){
      syslog(LOG_PRIORITY, "%s: name=*%s*, type=%s, size=%d, digest=%s", sdata->ttmpfile, state->attachments[i].filename, state->attachments[i].type, state->attachments[i].size, state->attachments[i].digest);

      counter = 0;

      p_bind_init(&sql);
      sql.sql[sql.pos] = state->attachments[i].digest; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

      if(p_exec_stmt(sdata, &sql) == ERR) goto CLOSE;

      p_bind_init(&sql);
      sql.sql[sql.pos] = (char *)&counter; sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;

      p_store_results(&sql);
      p_fetch_results(&sql);
      p_free_results(&sql);

      if(counter > 0){
         rc = AVIR_VIRUS;
         update_bad_attachment_counter(sdata, state->attachments[i].digest);
         syslog(LOG_PRIORITY, "%s: digest=%s is marked as malware (%d)", sdata->ttmpfile, state->attachments[i].digest, counter);
      }
   }


CLOSE:
   close_prepared_statement(&sql);

   return rc;
}
