/*
 * train.c, SJ
 */

#include <kospam.h>


// introduce new token with timestamp=NOW(), nham=0, nspam=0

int introduce_tokens(struct session_data *sdata, struct __state *state, struct __config *cfg){
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   if(state->n_token <= 0) return 1;

   get_tokens(state, -1, cfg);

   query = buffer_create(NULL);
   if(!query) return 0;

   snprintf(s, sizeof(s)-1, "INSERT INTO %s (token, nham, nspam) VALUES", SQL_TOKEN_TABLE);
   buffer_cat(query, s);

   n = 0;

   for(i=0; i<MAXHASH; i++){
      q = state->token_hash[i];
      while(q != NULL){

         if(q->nham + q->nspam == 0){
            if(n) snprintf(s, sizeof(s)-1, ",(%llu,0,0)", q->key);
            else snprintf(s, sizeof(s)-1, "(%llu,0,0)", q->key);
            buffer_cat(query, s);
            n++;
         }

         q = q->r;
      }
   }

   mysql_real_query(&(sdata->mysql), query->data, strlen(query->data));

   buffer_destroy(query);

   return 0;
}


int train_message(struct session_data *sdata, struct __state *state, char *column, struct __config *cfg){

   if(state->n_token <= 0) return 0;

   introduce_tokens(sdata, state, cfg);

   // update token nham or nspam column and the timestamp


    // insert all tokens to temp table

    char s[SMALLBUFSIZE];
    struct node *q;
    buffer *query;

    query = buffer_create(NULL);
    if(!query) return 1;

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

    buffer_cat(query, s);

    for(int i=0; i<MAXHASH; i++){
        q = state->token_hash[i];
        while(q != NULL){
           snprintf(s, sizeof(s)-1, ",(%llu)", q->key);

           buffer_cat(query, s);

           q = q->r;
        }
    }

    int rc = mysql_query(conn, query->data);

    buffer_destroy(query);

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


    // update misc table

    snprintf(s, sizeof(s)-1, "UPDATE %s SET %s=%s+1", SQL_MISC_TABLE, column, column);

    p_query(sdata, s);

    return 0;
}


/*
 * train this message
 */

void do_training(struct session_data *sdata, struct __state *state, char *email, struct __config *cfg){
   int is_spam = 0, is_spam_q = 0;
   struct sql sql;

   if(strcasestr(sdata->rcptto[0], "+spam@") || strncmp(email, "spam@", 5) == 0) is_spam = 1;


   // TODO: Add training in rounds

   /*
    * check if clapf_id exists in database
    */

   if(sdata->clapf_id[0] == '\0'){
      syslog(LOG_PRIORITY, "%s: error: missing signature", sdata->ttmpfile);
      return;
   }


   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_QUERY_TRAINING_ID) == ERR) return;

   p_bind_init(&sql);

   sql.sql[sql.pos] = sdata->clapf_id; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

   if(p_exec_stmt(sdata, &sql) == ERR) goto ENDE;

   p_bind_init(&sql);

   sql.sql[sql.pos] = (char *)&is_spam_q; sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;

   p_store_results(&sql);

   if(p_fetch_results(&sql) == ERR){
      syslog(LOG_PRIORITY, "%s: error: invalid signature '%s'", sdata->ttmpfile, sdata->clapf_id);
      return;
   }

   p_free_results(&sql);


   char s[SMALLBUFSIZE];
   if(is_spam) snprintf(s, sizeof(s)-1, "nspam");
   else snprintf(s, sizeof(s)-1, "nham");

   train_message(sdata, state, s, cfg);

   syslog(LOG_PRIORITY, "%s: training %s", sdata->ttmpfile, sdata->clapf_id);


ENDE:
   close_prepared_statement(&sql);

}
