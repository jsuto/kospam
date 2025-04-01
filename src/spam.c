/*
 * spam.c, SJ
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
 * calculate token probabilities
 */

void calcnode(struct node *xhash[], float NHAM, float NSPAM, struct __config *cfg){
   int i;
   struct node *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){

         if (q->nham >= 0 && q->nspam >= 0 && (q->nham + q->nspam) > 0) {
            int n = q->nham + q->nspam;
            if (n > 0) {
               q->spaminess = q->nspam * NHAM / (q->nspam * NHAM + q->nham * NSPAM);
               q->spaminess = (cfg->rob_s * cfg->rob_x + n * q->spaminess) / (cfg->rob_s + n);

               if(q->spaminess < REAL_HAM_TOKEN_PROBABILITY) q->spaminess = REAL_HAM_TOKEN_PROBABILITY;
               if(q->spaminess > REAL_SPAM_TOKEN_PROBABILITY) q->spaminess = REAL_SPAM_TOKEN_PROBABILITY;
            } else {
               q->spaminess = DEFAULT_SPAMICITY;
            }

            q->deviation = DEVIATION(q->spaminess);
         }

         q = q->r;
      }
   }
}


/*
 * query the spaminess values at once
 */


int qry_spaminess(struct session_data *sdata, struct __state *state, char type, struct __config *cfg){
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   query = buffer_create(NULL);
   if(!query) return 1;

   /*
    * CREATE TEMPORARY TABLE temp_tokens (token BIGINT UNSIGNED PRIMARY KEY);
    *
    * INSERT INTO temp_tokens (token) VALUES (...);
    *
    * EXPLAIN SELECT t.token, t.nham, t.nspam  FROM token t FORCE INDEX (token_uid_idx)  JOIN temp_tokens tt ON t.token = tt.token  WHERE t.uid IN (0,10);
    *
    * TRUNCATE temp_tokens; // after the update
    *
    *
    * EXPLAIN UPDATE token t FORCE INDEX (token_uid_idx) JOIN temp_tokens tt ON t.token = tt.token SET t.timestamp = 1743219892 WHERE t.uid IN (0,10);
    * +------+-------------+-------+--------+---------------+---------------+---------+---------------+--------+-------------+
    * | id   | select_type | table | type   | possible_keys | key           | key_len | ref           | rows   | Extra       |
    * +------+-------------+-------+--------+---------------+---------------+---------+---------------+--------+-------------+
    * |    1 | SIMPLE      | t     | range  | token_uid_idx | token_uid_idx | 2       | NULL          | 849817 | Using where |
    * |    1 | SIMPLE      | tt    | eq_ref | PRIMARY       | PRIMARY       | 8       | clapf.t.token | 1      | Using index |
    * +------+-------------+-------+--------+---------------+---------------+---------+---------------+--------+-------------+
    *
    * EXPLAIN UPDATE token t FORCE INDEX (token_uid_idx) JOIN temp_tokens tt ON t.token = tt.token SET t.timestamp = 1743219892 WHERE t.uid = 0;
    * +------+-------------+-------+-------+---------------+---------------+---------+----------------------+------+-------------+
    * | id   | select_type | table | type  | possible_keys | key           | key_len | ref                  | rows | Extra       |
    * +------+-------------+-------+-------+---------------+---------------+---------+----------------------+------+-------------+
    * |    1 | SIMPLE      | tt    | index | PRIMARY       | PRIMARY       | 8       | NULL                 | 6    | Using index |
    * |    1 | SIMPLE      | t     | ref   | token_uid_idx | token_uid_idx | 10      | const,clapf.tt.token | 1    |             |
    * +------+-------------+-------+-------+---------------+---------------+---------+----------------------+------+-------------+
    *
    * EXPLAIN UPDATE token t FORCE INDEX (token_uid_idx) JOIN temp_tokens tt ON t.token = tt.token SET t.timestamp = 1743219892 WHERE t.uid = 10;
    * +------+-------------+-------+--------+---------------+---------------+---------+---------------+------+-------------+
    * | id   | select_type | table | type   | possible_keys | key           | key_len | ref           | rows | Extra       |
    * +------+-------------+-------+--------+---------------+---------------+---------+---------------+------+-------------+
    * |    1 | SIMPLE      | t     | ref    | token_uid_idx | token_uid_idx | 2       | const         | 1    |             |
    * |    1 | SIMPLE      | tt    | eq_ref | PRIMARY       | PRIMARY       | 8       | clapf.t.token | 1    | Using index |
    * +------+-------------+-------+--------+---------------+---------------+---------+---------------+------+-------------+
    *
    *
    * Why Running Two Queries Might Be Better:

    Optimized Index Usage for uid = 0:

        Since the uid = 0 tokens are the majority, the query will be optimized for this subset, and it will not need to process the much smaller uid = 10 set.

        The UPDATE with uid = 0 will likely work on fewer rows in memory and will use the index efficiently, making it much faster.

    Lower Number of Rows Processed for uid = 10:

        Since there are fewer rows for uid = 10, the second query will perform more efficiently with less data to update, reducing the overall workload for MySQL.

    Fewer Locking Contention and Better Performance:

        By running two separate updates, the locking contention on the table will be lower. This is important if there are other operations happening on the table concurrently.

        This also minimizes the chance of a full table scan for each set, as MySQL can more efficiently optimize the query for the smaller result set (uid = 10).

    */

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

           if( (type == 1 && q->type == 1) || (type == 0 && q->type == 0) ){
              n++;
              snprintf(s, sizeof(s)-1, ",(%llu)", xxh3_64(q->str, strlen(q->str)));

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

    printf("INFO: %f, %f\n", sdata->nham, sdata->nspam);

    calcnode(state->token_hash, sdata->nham, sdata->nspam, cfg);

    return 0;
}


/*
 * update token timestamps
 */

int update_token_timestamps(struct session_data *sdata, struct node *xhash[]){
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   query = buffer_create(NULL);
   if(!query) return n;

   snprintf(s, sizeof(s)-1, "UPDATE %s SET timestamp=%ld WHERE (token=", SQL_TOKEN_TABLE, sdata->now);

   buffer_cat(query, s);

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(q->spaminess != DEFAULT_SPAMICITY){
            if(n) snprintf(s, sizeof(s)-1, " OR token=%llu", q->key);
            else snprintf(s, sizeof(s)-1, "%llu", q->key);
            buffer_cat(query, s);
            n++;
         }

         q = q->r;
      }
   }


   if(sdata->gid > 0)
      snprintf(s, sizeof(s)-1, ") AND (uid=0 OR uid=%d)", sdata->gid);
   else
      snprintf(s, sizeof(s)-1, ") AND uid=0");

   buffer_cat(query, s);

   if(mysql_real_query(&(sdata->mysql), query->data, strlen(query->data)) != 0){
      printf("update query: %s\n", query->data);
      n = -1;
   }

   buffer_destroy(query);

   return n;

}


double evaluate_tokens(struct session_data *sdata, struct __state *state, struct __config *cfg){
   int n_tokens=0, surbl_hit=0;
   float spaminess=DEFAULT_SPAMICITY;
   //int has_embed_image=0;

   if(cfg->debug == 1) printf("num of tokens: %d\n", state->n_token);

   if(cfg->penalize_embed_images == 1 && findnode(state->token_hash, "src+cid")){
      addnode(state->token_hash, "EMBED*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      //has_embed_image = 1;
   }


   /* calculate spaminess based on the token pairs and other special tokens */

   qry_spaminess(sdata, state, 1, cfg);

   add_penalties(sdata, state, cfg);

   spaminess = get_spam_probability(state->token_hash, &n_tokens, cfg);

   state->n_deviating_token = n_tokens;

   if(sdata->training_request == 1) return spaminess;


   if(cfg->debug == 1) printf("phrase: %.4f\n", spaminess);

   if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity) goto END_OF_EVALUATION;

   /* query the single tokens, then use the 'mix' for calculation */

   qry_spaminess(sdata, state, 0, cfg);
   spaminess = get_spam_probability(state->token_hash, &n_tokens, cfg);
   if(cfg->debug == 1) printf("mix: %.4f\n", spaminess);
   if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity) goto END_OF_EVALUATION;


   /* if we are still unsure, consult blacklists */

   surbl_hit = check_rbl_lists(state, cfg->surbl_domain);

   spaminess = get_spam_probability(state->token_hash, &n_tokens, cfg);
   if(cfg->debug == 1) printf("mix after blacklists: %.4f\n", spaminess);


   if(spaminess > cfg->max_ham_spamicity && spaminess < cfg->spam_overall_limit){
      if(surbl_hit > 0) spaminess = 0.99;
   }


END_OF_EVALUATION:

   /* fix spaminess value if we have to */

   if(spaminess < 0) spaminess = REAL_HAM_TOKEN_PROBABILITY;
   if(spaminess > 1) spaminess = REAL_SPAM_TOKEN_PROBABILITY;

   return spaminess;
}


float run_statistical_check(struct session_data *sdata, struct __state *state, struct __config *cfg){
   char buf[MAXBUFSIZE];
   float ham_from=0, spam_from=0;
   float spaminess = DEFAULT_SPAMICITY;
   struct te te;

   /* query message counters */

   if(cfg->group_type == GROUP_SHARED)
      snprintf(buf, sizeof(buf)-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);
   else
      snprintf(buf, sizeof(buf)-1, "SELECT nham, nspam FROM %s WHERE uid=0 OR uid=%d", SQL_MISC_TABLE, sdata->gid);

   te = get_ham_spam_counters(sdata, buf);
   sdata->nham = te.nham;
   sdata->nspam = te.nspam;

   if(sdata->nham + sdata->nspam == 0){
      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: %s", sdata->ttmpfile, ERR_SQL_DATA);
      return DEFAULT_SPAMICITY;
   }

   if(cfg->debug == 1) printf("nham: %.0f, nspam: %.0f\n", sdata->nham, sdata->nspam);


   /* check if sender is autowhitelisted */

   if(sdata->training_request == 0){

      snprintf(buf, sizeof(buf)-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND (uid=0 OR uid=%d)", SQL_TOKEN_TABLE, xxh3_64(state->from, strlen(state->from)), sdata->gid);

      te = get_ham_spam_counters(sdata, buf);
      ham_from = te.nham;
      spam_from = te.nspam;

      if(cfg->debug == 1) printf("from: %.0f, %.0f\n", ham_from, spam_from);

      if(ham_from >= NUMBER_OF_GOOD_FROM && spam_from == 0){
         if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: sender is statistically whitelisted", sdata->ttmpfile);

         /* query the spaminess of the token pairs in order to have their timestamp updated */
         qry_spaminess(sdata, state, 1, cfg);

         sdata->statistically_whitelisted = 1;

         return REAL_HAM_TOKEN_PROBABILITY;
      }
   }

   spaminess = evaluate_tokens(sdata, state, cfg);

   return spaminess;
}
