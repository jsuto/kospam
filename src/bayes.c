/*
 * bayes.c, SJ
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


/*
 * query the spaminess values at once
 */

int qry_spaminess(struct session_data *sdata, struct _state *state, char type, struct __config *cfg){
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   query = buffer_create(NULL);
   if(!query) return 0;

   snprintf(s, SMALLBUFSIZE-1, "SELECT token, nham, nspam FROM %s WHERE token in(%llu", SQL_TOKEN_TABLE, APHash(state->from));
   buffer_cat(query, s);

   for(i=0; i<MAXHASH; i++){
      q = state->token_hash[i];
      while(q != NULL){
         if( (type == 1 && q->type == 1) || (type == 0 && q->type == 0) ){
            n++;

         #ifdef HAVE_MYDB
            float spaminess = mydbqry(sdata, q->str, cfg);
            updatenode(state->token_hash, q->key, 0, 0, spaminess, DEVIATION(spaminess));
         #else
            snprintf(s, SMALLBUFSIZE-1, ",%llu", APHash(q->str));
            buffer_cat(query, s);
         #endif
         }

         q = q->r;
      }
   }

#ifndef HAVE_MYDB
   if(sdata->gid > 0){
      snprintf(s, SMALLBUFSIZE-1, ") AND (uid=0 OR uid=%ld)", sdata->gid);
      buffer_cat(query, s);
   }
   else
      buffer_cat(query, ") AND uid=0");

   update_hash(sdata, query->data, state->token_hash);
#endif

   buffer_destroy(query);

   calcnode(state->token_hash, sdata->Nham, sdata->Nspam, cfg);

   return 1;
}


double evaluateTokens(struct session_data *sdata, struct _state *state, struct __config *cfg){
   float spaminess=DEFAULT_SPAMICITY;
   int has_embed_image=0, found_on_rbl=0, surbl_match=0;


   if(cfg->penalize_embed_images == 1 && findnode(state->token_hash, "src+cid")){
      addnode(state->token_hash, "EMBED*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      has_embed_image = 1;
   }


   /* calculate spaminess based on the token pairs and other special tokens */

   qry_spaminess(sdata, state, 1, cfg);

   add_penalties(sdata, state, cfg);

   spaminess = getSpamProbabilityChi2(state->token_hash, cfg);

   if(sdata->training_request == 1) return spaminess;


   if(cfg->debug == 1) printf("phrase: %.4f\n", spaminess);

   if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity) goto END_OF_EVALUATION;

   /* query the single tokens, then use the 'mix' for calculation */

   qry_spaminess(sdata, state, 0, cfg);
   spaminess = getSpamProbabilityChi2(state->token_hash, cfg);
   if(cfg->debug == 1) printf("mix: %.4f\n", spaminess);
   if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity) goto END_OF_EVALUATION;


   /* if we are still unsure, consult blacklists */
 
#ifdef HAVE_RBL
   checkLists(sdata, state, &found_on_rbl, &surbl_match, cfg);
#endif

   spaminess = getSpamProbabilityChi2(state->token_hash, cfg);
   if(cfg->debug == 1) printf("mix after blacklists: %.4f\n", spaminess);



END_OF_EVALUATION:


   /* if the message is unsure, try to determine if it's a spam, 2008.01.09, SJ */

   if(spaminess > cfg->max_ham_spamicity && spaminess < cfg->spam_overall_limit)
      spaminess = applyPostSpaminessFixes(spaminess, found_on_rbl, surbl_match, has_embed_image, state->c_shit, state->l_shit, cfg);


   /* fix spaminess value if we have to */

   if(spaminess < 0) spaminess = REAL_HAM_TOKEN_PROBABILITY;
   if(spaminess > 1) spaminess = REAL_SPAM_TOKEN_PROBABILITY;

   return spaminess;
}


float bayes_file(struct session_data *sdata, struct _state *state, struct __config *cfg){
   char buf[MAXBUFSIZE], *p;
   float ham_from=0, spam_from=0;
   float spaminess = DEFAULT_SPAMICITY;
#ifdef HAVE_MYDB
   struct mydb_node *Q;
#else
   struct te TE;
#endif


   p = strrchr(sdata->ttmpfile, '/');
   if(p)
      p++;
   else
      p = sdata->ttmpfile;


#ifndef HAVE_MYSQL
   cfg->group_type = GROUP_SHARED;
#endif

   if(cfg->group_type == GROUP_SHARED) sdata->gid = 0;

   if(cfg->debug == 1) printf("username: %s, uid: %ld, gid: %ld\n", sdata->name, sdata->uid, sdata->gid);


   /* query message counters */

   if(cfg->group_type == GROUP_SHARED)
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);
   else
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0 OR uid=%ld", SQL_MISC_TABLE, sdata->gid);

#ifndef HAVE_MYDB
   TE = getHamSpamCounters(sdata, buf);
   sdata->Nham = TE.nham;
   sdata->Nspam = TE.nspam;
#endif

   if((sdata->Nham + sdata->Nspam == 0) && cfg->initial_1000_learning == 0){
      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: %s", p, ERR_SQL_DATA);
      return DEFAULT_SPAMICITY;
   }

   if(cfg->debug == 1) printf("nham: %.0f, nspam: %.0f\n", sdata->Nham, sdata->Nspam);


   /* check if sender is autowhitelisted */

   if(cfg->enable_auto_white_list == 1){

   #ifdef HAVE_MYDB
      Q = findmydb_node(sdata->mhash, APHash(state->from));
      if(Q){
         ham_from = Q->nham;
         spam_from = Q->nspam;
      }
   #else
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND (uid=0 OR uid=%ld)", SQL_TOKEN_TABLE, APHash(state->from), sdata->gid);

      TE = getHamSpamCounters(sdata, buf);
      ham_from = TE.nham;
      spam_from = TE.nspam;
   #endif

      if(cfg->debug == 1) printf("from: %.0f, %.0f\n", ham_from, spam_from);


      if(ham_from >= NUMBER_OF_GOOD_FROM && spam_from == 0){
         if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: sender is statistically whitelisted", sdata->ttmpfile);

         /* query the spaminess of the token pairs in order to have their timestamp updated */
         qry_spaminess(sdata, state, 1, cfg);

         sdata->statistically_whitelisted = 1;

         return REAL_HAM_TOKEN_PROBABILITY;
      }
   }


   spaminess = evaluateTokens(sdata, state, cfg);

   return spaminess;
}


int trainMessage(struct session_data *sdata, struct _state *state, int rounds, int is_spam, int train_mode, struct __config *cfg){
#ifdef HAVE_MYDB
   int rc;
#endif
   int i=0, n=0, rc, tm=train_mode;
   float spaminess = DEFAULT_SPAMICITY;

   if(counthash(state->token_hash) <= 0) return 0;

   if(cfg->group_type == GROUP_SHARED) sdata->gid = 0;

#ifndef HAVE_MYDB
   introduceTokens(sdata, state->token_hash);
#endif

   for(i=1; i<=rounds; i++){

      /* query the spaminess to see if we still have to train the message */

      resetcounters(state->token_hash);

   #ifdef HAVE_MYDB
      rc = init_mydb(cfg->mydbfile, sdata);
      if(rc == 1) spaminess = bayes_file(sdata, state, cfg);
      close_mydb(sdata->mhash);
   #else
      spaminess = bayes_file(sdata, state, cfg);
   #endif

      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: training %d, round: %d spaminess was: %0.4f", sdata->ttmpfile, is_spam, n, spaminess);

      if(is_spam == 1 && spaminess > 0.99) break;
      if(is_spam == 0 && spaminess < 0.1) break;

      /* then update the counters */

   #ifdef HAVE_MYDB
      my_walk_hash(cfg->mydbfile, sdata->mhash, is_spam, state->token_hash, tm);
   #else
      rc = updateTokenCounters(sdata, is_spam, state->token_hash, T_TOE);
      updateMiscTable(sdata, is_spam, T_TOE);

      /* fix counters if it was a TUM training */

      if(tm == T_TUM){
         rc = updateTokenCounters(sdata, is_spam, state->token_hash, T_TUM);
         updateMiscTable(sdata, is_spam, T_TUM);
      }
   #endif

      n++;

      tm = T_TOE;
   }


   return n;
}


void add_penalties(struct session_data *sdata, struct _state *state, struct __config *cfg){

   /* add a spammy token if we got a binary, eg. PDF attachment, 2007.07.02, SJ */

   if(cfg->penalize_octet_stream == 1 && (attachment_by_type(state, "application/octet-stream") == 1 || attachment_by_type(state, "application/pdf") == 1
       || attachment_by_type(state, "application/vnd.ms-excel") == 1
       || attachment_by_type(state, "application/msword") == 1
       || attachment_by_type(state, "application/rtf") == 1
       || attachment_by_type(state, "application/x-zip-compressed") == 1)
   ){
       addnode(state->token_hash, "OCTET_STREAM*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
   }

   /* add penalty for images, 2007.07.02, SJ */

   if(cfg->penalize_images == 1 && attachment_by_type(state, "image/") == 1)
       addnode(state->token_hash, "IMAGE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(state->n_subject_token == 0)
      addnode(state->token_hash, "NO_SUBJECT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(strcmp(state->hostname, "unknown") == 0 && sdata->Nham > NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED)
      addnode(state->token_hash, "UNKNOWN_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(sdata->trapped_client == 1)
      addnode(state->token_hash, "TRAPPED_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(sdata->tre == '+')
      addnode(state->token_hash, "ZOMBIE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

}

