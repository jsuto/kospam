/*
 * history.c, SJ
 */

#include <kospam.h>


int store_file_to_quarantine(char *filename, struct config *cfg){
   int rc=ERR;
   char *p0, *p1, *p2, s[SMALLBUFSIZE];
   struct stat st;

   /* create a filename in the store based on piler_id */

   snprintf(s, sizeof(s)-1, "%s/%02x/%c%c/%c%c/%s", cfg->quarantinedir, cfg->server_id, filename[RND_STR_LEN-4], filename[RND_STR_LEN-3], filename[RND_STR_LEN-2], filename[RND_STR_LEN-1], filename);

   p0 = strrchr(s, '/'); if(!p0) return rc;
   *p0 = '\0';

   if(stat(s, &st)){
      p1 = strrchr(s, '/'); if(!p1) return rc;
      *p1 = '\0';
      p2 = strrchr(s, '/'); if(!p2) return rc;
      *p2 = '\0';

      rc = mkdir(s, 0755);
      *p2 = '/';
      rc = mkdir(s, 0755);
      *p1 = '/';
      rc = mkdir(s, 0755); if(rc == -1) syslog(LOG_PRIORITY, "%s: mkdir %s: error=%s", filename, s, strerror(errno));
   }

   *p0 = '/';

   rc = ERR;

   if(link(filename, s) == 0) rc = OK;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: saving to queue: %s", filename, s);

   return rc;
}

int write_history_to_sql(struct session_data *sdata, MYSQL *conn, struct parser_state *state){
   int rc=ERR;
   struct query sql;

   if(prepare_sql_statement(conn, &sql, SQL_PREPARED_STMT_INSERT_INTO_HISTORY) == ERR) return rc;

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

   if(p_exec_stmt(conn, &sql) == OK) rc = OK;

   close_prepared_statement(&sql);

   return rc;
}
