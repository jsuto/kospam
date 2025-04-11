/*
 * qqq.c, SJ
 */

#include <kospam.h>


/*
 * query tokens from sql table using a temp table
 */


int get_tokens(struct parser_state *state, char type, struct config *cfg){
    int i, n=0;
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

    // The string representation of the largest unit64 number (18446744073709551615) is 21 character long
    // Add +3 bytes for ",(" and ")", that's 24 characters, so we need a buffer size like n_token * 24 + SMALLBUFSIZE

    size_t len = SMALLBUFSIZE + state->n_token * 24;
    if (cfg->debug) printf("allocating %ld bytes for %d tokens\n", len, state->n_token);
    char *query = malloc(len);
    if (!query) {
        syslog(LOG_PRIORITY, "ERROR: malloc() error %s", __func__);
        mysql_close(conn);
        return 1;
    }

    memset(query, 0, len);
    size_t pos = 0;

    snprintf(s, sizeof(s)-1, "INSERT INTO %s (token) VALUES (0)", SQL_TEMP_TOKEN_TABLE);
    len = strlen(s);
    memcpy(query+pos, s, len);
    pos += len;

    for(i=0; i<MAXHASH; i++){
        q = state->token_hash[i];
        while(q != NULL){

           if( type == -1 || (type == 1 && q->type == 1) || (type == 0 && q->type == 0) ){
              n++;
              snprintf(s, sizeof(s)-1, ",(%llu)", xxh3_64(q->str));
              len = strlen(s);
              memcpy(query+pos, s, len);
              pos += len;
           }

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

    // Query the main token table using a JOIN
    const char *select_query =
        "SELECT t.token, t.nham, t.nspam FROM " SQL_TOKEN_TABLE " t "
        "JOIN " SQL_TEMP_TOKEN_TABLE " lt ON t.token = lt.token";

    if (mysql_query(conn, select_query) != 0) {
        syslog(LOG_PRIORITY, "ERROR: %s", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        syslog(LOG_PRIORITY, "ERROR: %s", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        updatenode(state->token_hash, strtoull(row[0], NULL, 10), atof(row[1]), atof(row[2]), DEFAULT_SPAMICITY, 0);
    }

    mysql_free_result(result);

    mysql_close(conn);

    return 0;
}


int update_token_dates(struct parser_state *state, struct config *cfg) {
    int utokens = 0;
    char s[SMALLBUFSIZE];
    struct node *q;

    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        syslog(LOG_PRIORITY, "ERROR: mysql_init() failed");
        return utokens;
    }

    if (!mysql_real_connect(conn, cfg->mysqlhost, cfg->mysqluser, cfg->mysqlpwd, cfg->mysqldb, cfg->mysqlport, cfg->mysqlsocket, 0)) {
        syslog(LOG_PRIORITY, "ERROR: cant connect to mysql server: '%s', error: %s", cfg->mysqldb, mysql_error(conn));
        mysql_close(conn);
        return utokens;
    }

    // Create a temporary table
    snprintf(s, sizeof(s)-1, "CREATE TEMPORARY TABLE %s (token BIGINT UNSIGNED PRIMARY KEY)", SQL_TEMP_TOKEN_TABLE);

    if (mysql_query(conn, s)) {
        syslog(LOG_PRIORITY, "ERROR: %s", mysql_error(conn));
        mysql_close(conn);
        return utokens;
    }

    snprintf(s, sizeof(s)-1, "INSERT INTO %s (token) VALUES (0)", SQL_TEMP_TOKEN_TABLE);

    int count = count_existing_tokens_in_token_table(state->token_hash);

    size_t len = SMALLBUFSIZE + count * 24;
    if (cfg->debug) printf("allocating %ld bytes for %d tokens\n", len, count);
    char *query = malloc(len);
    if (!query) {
        syslog(LOG_PRIORITY, "ERROR: malloc() error %s", __func__);
        mysql_close(conn);
        return utokens;
    }

    memset(query, 0, len);
    size_t pos = 0;

    len = strlen(s);
    memcpy(query+pos, s, len);
    pos += len;

    for(int i=0; i<MAXHASH; i++){
        q = state->token_hash[i];
        while(q != NULL) {
            if (q->nham + q->nspam > 0) {
               utokens++;
               snprintf(s, sizeof(s)-1, ",(%llu)", q->key);
               len = strlen(s);
               memcpy(query+pos, s, len);
               pos += len;
            }

            q = q->r;
        }
    }

    if (cfg->debug) printf("update date query: *%s*\n", query);

    int rc = mysql_query(conn, query);

    if (query) free(query);

    if (rc != 0) {
        syslog(LOG_PRIORITY, "ERROR: %s", mysql_error(conn));
        mysql_close(conn);
        return utokens;
    }

    snprintf(s, sizeof(s)-1, "UPDATE %s t JOIN %s tt ON t.token = tt.token SET updated=NOW()", SQL_TOKEN_TABLE, SQL_TEMP_TOKEN_TABLE);

    if (mysql_query(conn, s) != 0) {
        syslog(LOG_PRIORITY, "ERROR: %s", mysql_error(conn));
    }

    mysql_close(conn);

    return utokens;
}
