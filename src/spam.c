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
#include <clapf.h>


float calc_token_spamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x){
   float spamicity=DEFAULT_SPAMICITY;
   int n;

   n = nham + nspam;
   if(n <= 0) return DEFAULT_SPAMICITY;

   spamicity = nspam * NHAM / (nspam * NHAM + nham * NSPAM);
   spamicity = (rob_s * rob_x + n * spamicity) / (rob_s + n);

   if(spamicity < REAL_HAM_TOKEN_PROBABILITY) spamicity = REAL_HAM_TOKEN_PROBABILITY;
   if(spamicity > REAL_SPAM_TOKEN_PROBABILITY) spamicity = REAL_SPAM_TOKEN_PROBABILITY;

   return spamicity;
}


/*
 * calculate token probabilities
 */

void calcnode(struct node *xhash[], float nham, float nspam, struct __config *cfg){
   int i;
   struct node *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){

         if(q->nham >= 0 && q->nspam >= 0 && (q->nham + q->nspam) > 0){
            q->spaminess = calc_token_spamicity(nham, nspam, q->nham, q->nspam, cfg->rob_s, cfg->rob_x);
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
   if(!query) return 0;

   /*
    * I've noticed recently that mysql has a hard time executing queries like
    * SELECT token, nham, nspam FROM token WHERE token IN ( .... );
    *
    * The problem is that mysql uses a full table scan in such case and won't use
    * any index. The workaround seems to be rewriting the query:
    *
    * SELECT token, nham, nspam FROM token WHERE (token=a OR token=b OR token=c ...);
    */

   snprintf(s, sizeof(s)-1, "SELECT token, nham, nspam FROM %s WHERE (token=%llu", SQL_TOKEN_TABLE, APHash(state->from));
   buffer_cat(query, s);


   for(i=0; i<MAXHASH; i++){
      q = state->token_hash[i];
      while(q != NULL){

         if( (type == 1 && q->type == 1) || (type == 0 && q->type == 0) ){
            n++;
            snprintf(s, sizeof(s)-1, " OR token=%llu", APHash(q->str));

            buffer_cat(query, s);
         }

         q = q->r;
      }
   }

   if(sdata->gid > 0){
      snprintf(s, sizeof(s)-1, ") AND (uid=0 OR uid=%d)", sdata->gid);
      buffer_cat(query, s);
   }
   else
      buffer_cat(query, ") AND uid=0");

   update_hash(sdata, query->data, state->token_hash);

   buffer_destroy(query);

   calcnode(state->token_hash, sdata->nham, sdata->nspam, cfg);

   return 1;
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

   snprintf(s, sizeof(s)-1, "UPDATE %s SET timestamp=%ld WHERE token=", SQL_TOKEN_TABLE, sdata->now);

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

   surbl_hit = check_rbl_lists(state, cfg->surbl_domain, cfg);

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

      snprintf(buf, sizeof(buf)-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND (uid=0 OR uid=%d)", SQL_TOKEN_TABLE, APHash(state->from), sdata->gid);

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


