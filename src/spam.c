/*
 * spam.c, SJ
 */

#include <kospam.h>


/*
 * pull tokens from database and calculate spam probabilities
 */

void qry_spaminess(struct session_data *sdata, struct parser_state *state, char type, struct config *cfg){
   get_tokens(state, type, cfg);

   for(int i=0;i<MAXHASH;i++){
      struct node *q = state->token_hash[i];
      while(q != NULL){

         if (q->nham >= 0 && q->nspam >= 0 && (q->nham + q->nspam) > 0) {
            int n = q->nham + q->nspam;
            if (n > 0) {
               q->spaminess = q->nspam * sdata->nham / (q->nspam * sdata->nham + q->nham * sdata->nspam);
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


double evaluate_tokens(struct session_data *sdata, struct parser_state *state, struct config *cfg){
   int n_tokens=0;
   float spaminess=DEFAULT_SPAMICITY;

   if(cfg->debug == 1) printf("num of tokens: %d\n", state->n_token);

   if(cfg->penalize_embed_images == 1 && findnode(state->token_hash, "src+cid")){
      addnode(state->token_hash, "EMBED*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
   }


   /* calculate spaminess based on the token pairs and other special tokens */

   qry_spaminess(sdata, state, 1, cfg);

   add_penalties(sdata, state, cfg);

   spaminess = get_spam_probability(state->token_hash, &n_tokens, cfg);

   state->n_deviating_token = n_tokens;

   if(state->training_request == 1) return spaminess;


   if(cfg->debug == 1) printf("phrase: %.4f\n", spaminess);

   if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity) goto END_OF_EVALUATION;

   /* query the single tokens, then use the 'mix' for calculation */

   qry_spaminess(sdata, state, 0, cfg);
   spaminess = get_spam_probability(state->token_hash, &n_tokens, cfg);
   if(cfg->debug == 1) printf("mix: %.4f\n", spaminess);
   if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity) goto END_OF_EVALUATION;


   /* if we are still unsure, consult blacklists */

   spaminess = get_spam_probability(state->token_hash, &n_tokens, cfg);
   if(cfg->debug == 1) printf("mix after blacklists: %.4f\n", spaminess);


END_OF_EVALUATION:

   /* fix spaminess value if we have to */

   if(spaminess < 0) spaminess = REAL_HAM_TOKEN_PROBABILITY;
   if(spaminess > 1) spaminess = REAL_SPAM_TOKEN_PROBABILITY;

   return spaminess;
}


float run_statistical_check(struct session_data *sdata, struct parser_state *state, MYSQL *conn, struct config *cfg){
   char buf[MAXBUFSIZE];
   float ham_from=0, spam_from=0;
   float spaminess = DEFAULT_SPAMICITY;
   struct te te;

   /* query message counters */

   snprintf(buf, sizeof(buf)-1, "SELECT nham, nspam FROM %s", SQL_MISC_TABLE);

   te = get_ham_spam_counters(conn, buf);
   sdata->nham = te.nham;
   sdata->nspam = te.nspam;

   if(sdata->nham + sdata->nspam == 0){
      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: %s", sdata->ttmpfile, ERR_SQL_DATA);
      return DEFAULT_SPAMICITY;
   }

   if(cfg->debug == 1) printf("nham: %.0f, nspam: %.0f\n", sdata->nham, sdata->nspam);


   /* check if sender is autowhitelisted */

   if(state->training_request == 0){

      snprintf(buf, sizeof(buf)-1, "SELECT nham, nspam FROM %s WHERE token=%llu", SQL_TOKEN_TABLE, xxh3_64(state->from));

      te = get_ham_spam_counters(conn, buf);
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
