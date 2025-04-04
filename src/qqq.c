/*
 * qqq.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <kospam.h>


/*
 * query tokens from sql table using a temp table
 */


int get_tokens(struct __state *state, char type, struct __config *cfg){
    int i, n=0;
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

    for(i=0; i<MAXHASH; i++){
        q = state->token_hash[i];
        while(q != NULL){

           if( type == -1 || (type == 1 && q->type == 1) || (type == 0 && q->type == 0) ){
              n++;
              snprintf(s, sizeof(s)-1, ",(%llu)", xxh3_64(q->str));

              buffer_cat(query, s);
           }

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


int update_token_dates(struct __state *state, struct __config *cfg) {
    int utokens = 0;
    char s[SMALLBUFSIZE];
    struct node *q;
    buffer *query;

    query = buffer_create(NULL);
    if(!query) return utokens;

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

    buffer_cat(query, s);

    for(int i=0; i<MAXHASH; i++){
        q = state->token_hash[i];
        while(q != NULL) {
            if (q->nham + q->nspam > 0) {
               utokens++;
               snprintf(s, sizeof(s)-1, ",(%llu)", q->key);
               //printf("%llu, %f, %f\n", q->key, q->nham, q->nspam);
               buffer_cat(query, s);
            }

            q = q->r;
        }
    }

    //if (cfg->debug) printf("update date query: *%s*\n", query->data);

    int rc = mysql_query(conn, query->data);

    buffer_destroy(query);

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
