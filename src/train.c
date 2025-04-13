/*
 * train.c, SJ
 */

#include <kospam.h>


// introduce new token with timestamp=NOW(), nham=0, nspam=0

int introduce_tokens(MYSQL *conn, struct parser_state *state, struct config *cfg){
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *q;

   if(state->n_token <= 0) return 1;

   get_tokens(state, -1, cfg);

   snprintf(s, sizeof(s)-1, "INSERT INTO %s (token, nham, nspam) VALUES", SQL_TOKEN_TABLE);

   // The string representation of the largest unit64 number (18446744073709551615) is 21 character long
   // Add +7 bytes for ",(" and ",0,0)", that's 28 characters, so we need a buffer size like n_token * 28 + SMALLBUFSIZE

   size_t len = SMALLBUFSIZE + state->n_token * 28;
   if (cfg->debug) printf("allocating %ld bytes for %d new tokens\n", len, state->n_token);
   char *query = malloc(len);
   if (!query) {
       syslog(LOG_PRIORITY, "ERROR: malloc() error %s", __func__);
       mysql_close(conn);
       return 1;
   }

   memset(query, 0, len);
   size_t pos = 0;

   len = strlen(s);
   memcpy(query+pos, s, len);
   pos += len;

   n = 0;

   for(i=0; i<MAXHASH; i++){
      q = state->token_hash[i];
      while(q != NULL){

         if(q->nham + q->nspam == 0){
            if(n) snprintf(s, sizeof(s)-1, ",(%llu,0,0)", q->key);
            else snprintf(s, sizeof(s)-1, "(%llu,0,0)", q->key);
            len = strlen(s);
            memcpy(query+pos, s, len);
            pos += len;

            n++;
         }

         q = q->r;
      }
   }

   if (cfg->debug) printf("query: *%s*\n", query);

   mysql_real_query(conn, query, pos);

   if (query) free(query);

   return 0;
}


int train_message(struct parser_state *state, char *column, struct config *cfg){

    if(state->n_token <= 0) return 0;

    // insert all tokens to temp table

    char s[SMALLBUFSIZE];
    struct node *q;

    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        syslog(LOG_PRIORITY, "ERROR: mysql_init() failed");
        return 1;
    }

    if (!mysql_real_connect(conn, cfg->mysqlhost, cfg->mysqluser, cfg->mysqlpwd, cfg->mysqldb, cfg->mysqlport, cfg->mysqlsocket, 0)) {
        syslog(LOG_PRIORITY, "ERROR: cant connect to mysql server: '%s', error: %s", cfg->mysqldb, mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    // Create a temporary table
    snprintf(s, sizeof(s)-1, "CREATE TEMPORARY TABLE %s (token BIGINT UNSIGNED PRIMARY KEY)", SQL_TEMP_TOKEN_TABLE);

    if (mysql_query(conn, s)) {
        syslog(LOG_PRIORITY, "ERROR: %s", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    snprintf(s, sizeof(s)-1, "INSERT INTO %s (token) VALUES (0)", SQL_TEMP_TOKEN_TABLE);

    size_t len = SMALLBUFSIZE + state->n_token * 24;
    if (cfg->debug) printf("allocating %ld bytes for updating %d tokens\n", len, state->n_token);
    char *query = malloc(len);
    if (!query) {
        syslog(LOG_PRIORITY, "ERROR: malloc() error %s", __func__);
        mysql_close(conn);
        return 1;
    }

    memset(query, 0, len);
    size_t pos = 0;

    len = strlen(s);
    memcpy(query+pos, s, len);
    pos += len;

    for(int i=0; i<MAXHASH; i++){
        q = state->token_hash[i];
        while(q != NULL){
           snprintf(s, sizeof(s)-1, ",(%llu)", q->key);

           len = strlen(s);
           memcpy(query+pos, s, len);
           pos += len;

           q = q->r;
        }
    }

    if (cfg->debug) printf("query: *%s*\n", query);

    int rc = mysql_query(conn, query);

    if (query) free(query);

    if (rc != 0) {
        syslog(LOG_PRIORITY, "ERROR: %s", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    // all tokens are in the temp table
    // run update on the main token table using a JOIN

    snprintf(s, sizeof(s)-1, "UPDATE %s t JOIN %s tt ON t.token = tt.token SET %s=%s+1, updated=NOW()", SQL_TOKEN_TABLE, SQL_TEMP_TOKEN_TABLE, column, column);

    if (mysql_query(conn, s) != 0) {
        syslog(LOG_PRIORITY, "ERROR: %s", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }


    // update the token counters in misc table

    snprintf(s, sizeof(s)-1, "UPDATE %s SET %s=%s+1", SQL_MISC_TABLE, column, column);
    p_query(conn, s);

    mysql_close(conn);

    return 0;
}


/*
 * train this message
 */

void do_training(struct session_data *sdata, struct parser_state *state, MYSQL *conn, struct config *cfg){
   int is_spam = 0, is_spam_q = 0;
   struct query sql;

   if(strcasestr(state->envelope_recipient, "+spam@") || strncmp(state->envelope_recipient, "spam@", 5) == 0) is_spam = 1;


   /*
    * check if the Kospam watermark exists in database
    */

   if(state->kospam_watermark[0] == '\0'){
      syslog(LOG_PRIORITY, "%s: ERROR: missing signature", sdata->ttmpfile);
      return;
   }


   if(prepare_sql_statement(conn, &sql, SQL_PREPARED_STMT_QUERY_TRAINING_ID) == ERR) return;

   p_bind_init(&sql);

   sql.sql[sql.pos] = state->kospam_watermark; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

   if(p_exec_stmt(conn, &sql) == ERR) goto ENDE;

   p_bind_init(&sql);

   sql.sql[sql.pos] = (char *)&is_spam_q; sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;

   p_store_results(&sql);

   if(p_fetch_results(&sql) == ERR){
      syslog(LOG_PRIORITY, "%s: ERROR: invalid signature '%s'", sdata->ttmpfile, state->kospam_watermark);
      return;
   }

   p_free_results(&sql);


   char s[SMALLBUFSIZE];
   if(is_spam) snprintf(s, sizeof(s)-1, "nspam");
   else snprintf(s, sizeof(s)-1, "nham");

   // Add new tokens first
   introduce_tokens(conn, state, cfg);

   for (int i=0; i<MAX_ITERATIVE_TRAIN_LOOPS; i++) {

      resetcounters(state->token_hash);

      sdata->spaminess = run_statistical_check(sdata, state, conn, cfg);

      if(/*state->n_deviating_token > 20 &&*/ is_spam == 1 && sdata->spaminess > 0.99) break;
      if(/*state->n_deviating_token > 20 &&*/ is_spam == 0 && sdata->spaminess < 0.1) break;

      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: training %d, round: %d spaminess: %0.4f, deviating tokens: %d", sdata->ttmpfile, is_spam, i, sdata->spaminess, state->n_deviating_token);

      train_message(state, s, cfg);
   }

ENDE:
   close_prepared_statement(&sql);

}
