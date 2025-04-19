/*
 * attachment.c, SJ
 */

#include <kospam.h>


void update_bad_attachment_counter(MYSQL *conn, char *digest){
   struct query sql;

   if (prepare_sql_statement(conn, &sql, SQL_PREPARED_STMT_UPDATE_ADIGEST) == OK) {

      p_bind_init(&sql);
      sql.sql[sql.pos] = digest; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

      p_exec_stmt(conn, &sql);

      close_prepared_statement(&sql);
   }
}


bool has_known_bad_attachment(MYSQL *conn, struct session_data *sdata, struct Message *m) {
   bool rc = false;

   if (m->n_attachments == 0) return rc;

   struct query sql;

   if(prepare_sql_statement(conn, &sql, SQL_PREPARED_STMT_QUERY_ADIGEST) == ERR) return rc;

   for (int i=0; i<m->n_attachments; i++) {
      syslog(LOG_PRIORITY, "%s: name=%s, type=%s, size=%ld, digest=%s", sdata->ttmpfile, m->attachments[i].filename, m->attachments[i].type, m->attachments[i].size, m->attachments[i].digest);

      uint64 counter = 0;

      p_bind_init(&sql);
      sql.sql[sql.pos] = m->attachments[i].digest; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

      if(p_exec_stmt(conn, &sql) == ERR) goto CLOSE;

      p_bind_init(&sql);
      sql.sql[sql.pos] = (char *)&counter; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;

      p_store_results(&sql);
      p_fetch_results(&sql);
      p_free_results(&sql);

      if (counter > 0) {
         rc = true;
         update_bad_attachment_counter(conn, m->attachments[i].digest);
         syslog(LOG_PRIORITY, "%s: digest=%s is marked as malware (%llu)", sdata->ttmpfile, m->attachments[i].digest, counter);
      }
   }


CLOSE:
   close_prepared_statement(&sql);

   return rc;
}
