/*
 * train.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <clapf.h>


int update_token_counters(struct session_data *sdata, struct __state *state, int ham_or_spam, struct node *xhash[], int train_mode){
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   if(state->n_token <= 0) return 0;

   query = buffer_create(NULL);
   if(!query) return n;

   if(ham_or_spam == 1){
      if(train_mode == T_TUM) snprintf(s, sizeof(s)-1, "UPDATE %s SET nham=nham-1 WHERE token IN (", SQL_TOKEN_TABLE);
      else snprintf(s, sizeof(s)-1, "UPDATE %s SET nspam=nspam+1 WHERE token IN (", SQL_TOKEN_TABLE);
   } else {
      if(train_mode == T_TUM) snprintf(s, sizeof(s)-1, "UPDATE %s SET nspam=nspam-1 WHERE token IN (", SQL_TOKEN_TABLE);
      else snprintf(s, sizeof(s)-1, "UPDATE %s SET nham=nham+1 WHERE token IN (", SQL_TOKEN_TABLE);
   }

   buffer_cat(query, s);

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(n) snprintf(s, sizeof(s)-1, ",%llu", q->key);
         else snprintf(s, sizeof(s)-1, "%llu", q->key);

         buffer_cat(query, s);

         q = q->r;
         n++;
      }
   }

   buffer_cat(query, ")");

   if(train_mode == T_TUM){
      if(ham_or_spam == 1) buffer_cat(query, " AND nham > 0");
      else buffer_cat(query, " AND nspam > 0");
   }

   snprintf(s, sizeof(s)-1, " AND uid=%d", sdata->gid);
   buffer_cat(query, s);

   p_query(sdata, query->data);

   buffer_destroy(query);

   return 1;
}


int update_misc_table(struct session_data *sdata, int ham_or_spam, int train_mode){
   char s[SMALLBUFSIZE];

   if(ham_or_spam == 1){
      if(train_mode == T_TUM) snprintf(s, sizeof(s)-1, "UPDATE %s SET nham=nham-1 WHERE uid=%d AND nham > 0", SQL_MISC_TABLE, sdata->gid);
      else snprintf(s, sizeof(s)-1, "UPDATE %s SET nspam=nspam+1 WHERE uid=%d", SQL_MISC_TABLE, sdata->gid);
   } else {
      if(train_mode == T_TUM) snprintf(s, sizeof(s)-1, "UPDATE %s SET nspam=nspam-1 WHERE uid=%d AND nspam > 0", SQL_MISC_TABLE, sdata->gid);
      else snprintf(s, sizeof(s)-1, "UPDATE %s SET nham=nham+1 WHERE uid=%d", SQL_MISC_TABLE, sdata->gid);
   }

   p_query(sdata, s);

   return 1;
}


int introduce_tokens(struct session_data *sdata, struct __state *state, struct node *xhash[]){
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   if(state->n_token <= 0) return 0;

   query = buffer_create(NULL);
   if(!query) return 0;

   snprintf(s, sizeof(s)-1, "SELECT token, nham, nspam FROM %s WHERE token in (", SQL_TOKEN_TABLE);
   buffer_cat(query, s);

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(n) snprintf(s, sizeof(s)-1, ",%llu", q->key);
         else snprintf(s, sizeof(s)-1, "%llu", q->key);

         buffer_cat(query, s);
         n++;

         q = q->r;
      }
   }

   snprintf(s, sizeof(s)-1, ") AND uid=%d", sdata->gid);
   buffer_cat(query, s);

   update_hash(sdata, query->data, xhash);

   buffer_destroy(query);


   query = buffer_create(NULL);
   if(!query) return 0;

   snprintf(s, sizeof(s)-1, "INSERT INTO %s (token, nham, nspam, uid, timestamp) VALUES", SQL_TOKEN_TABLE);
   buffer_cat(query, s);

   n = 0;

   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         if(q->nham + q->nspam == 0){
            if(n) snprintf(s, sizeof(s)-1, ",(%llu,0,0,%d,%ld)", q->key, sdata->gid, sdata->now);
            else snprintf(s, sizeof(s)-1, "(%llu,0,0,%d,%ld)", q->key, sdata->gid, sdata->now);

            buffer_cat(query, s);
            n++;
         }

         q = q->r;
      }
   }

   mysql_real_query(&(sdata->mysql), query->data, strlen(query->data));

   buffer_destroy(query);

   return 1;
}


int train_message(struct session_data *sdata, struct __state *state, int rounds, int is_spam, int train_mode, struct __config *cfg){
   int i=0, n=0, tm=train_mode;

   if(state->n_token <= 0) return 0;

   if(cfg->group_type == GROUP_SHARED) sdata->gid = 0;

   introduce_tokens(sdata, state, state->token_hash);

   for(i=1; i<=rounds; i++){

      /* query the spaminess to see if we still have to train the message */

      resetcounters(state->token_hash);

      sdata->spaminess = run_statistical_check(sdata, state, cfg);

      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: training %d, round: %d spaminess was: %0.4f, deviating tokens: %d", sdata->ttmpfile, is_spam, n, sdata->spaminess, state->n_deviating_token);

      /*
       * don't skip the training if the message has <20 deviating token
       */

      if(state->n_deviating_token > 20 && is_spam == 1 && sdata->spaminess > 0.99) break;
      if(state->n_deviating_token > 20 && is_spam == 0 && sdata->spaminess < 0.1) break;


      /* then update the counters */

      update_token_counters(sdata, state, is_spam, state->token_hash, T_TOE);
      update_misc_table(sdata, is_spam, T_TOE);

      /* fix counters if it was a TUM training */

      if(tm == T_TUM){
         update_token_counters(sdata, state, is_spam, state->token_hash, T_TUM);
         update_misc_table(sdata, is_spam, T_TUM);
      }

      n++;

      tm = T_TOE;
   }


   return n;
}


/*
 * train this message
 */

void do_training(struct session_data *sdata, struct __state *state, char *email, struct __config *cfg){
   int i, is_spam = 0, is_spam_q = 0;
   struct sql sql;

   if(strcasestr(sdata->rcptto[0], "+spam@") || strncmp(email, "spam@", 5) == 0) is_spam = 1;


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

   p_store_results(sdata, &sql);

   if(p_fetch_results(&sql) == ERR){
      syslog(LOG_PRIORITY, "%s: error: invalid signature '%s'", sdata->ttmpfile, sdata->clapf_id);
      return;
   }

   p_free_results(&sql);


   // FIXME: fix sdata->gid = 0 in case of global training request


   i = train_message(sdata, state, MAX_ITERATIVE_TRAIN_LOOPS, is_spam, state->train_mode, cfg);

   syslog(LOG_PRIORITY, "%s: training %s in %d rounds", sdata->ttmpfile, sdata->clapf_id, i);


ENDE:
   close_prepared_statement(&sql);

}


