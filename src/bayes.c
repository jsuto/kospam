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
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "messages.h"
#include "score.h"
#include "bayes.h"
#include "config.h"
#include "buffer.h"
#include "clapf.h"

#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL_RES *res;
   MYSQL_ROW row;
   int mysql_conn = 0;
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
#endif

#ifdef HAVE_MYDB
   #include "mydb.h"
#endif


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

#ifdef HAVE_MYSQL
   if(sdata->uid > 0){
      snprintf(s, SMALLBUFSIZE-1, ") AND (uid=0 OR uid=%ld)", sdata->uid);
      buffer_cat(query, s);
   }
   else
      buffer_cat(query, ") AND uid=0");

   update_hash(sdata, query->data, state->token_hash);
#endif
#ifdef HAVE_SQLITE3
   buffer_cat(query, ")");
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
   check_lists(sdata, state, &found_on_rbl, &surbl_match, cfg);
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

   if(cfg->debug == 1) printf("username: %s, uid: %ld\n", sdata->name, sdata->uid);


   /* query message counters */

   if(cfg->group_type == GROUP_SHARED)
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);
   else
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0 OR uid=%ld", SQL_MISC_TABLE, sdata->uid);

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
      #ifdef HAVE_MYSQL
         snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND (uid=0 OR uid=%ld)", SQL_TOKEN_TABLE, APHash(state->from), sdata->uid);
      #endif
      #ifdef HAVE_SQLITE3
         snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu", SQL_TOKEN_TABLE, APHash(state->from));
      #endif
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


   int saved_uid = sdata->uid;

   if(cfg->group_type == GROUP_SHARED) sdata->uid = 0;

   spaminess = evaluateTokens(sdata, state, cfg);

   /* restore saved uid */
   if(cfg->group_type == GROUP_SHARED) sdata->uid = saved_uid;

   return spaminess;
}


int trainMessage(struct session_data *sdata, struct _state *state, int rounds, int is_spam, int train_mode, struct __config *cfg){
#ifdef HAVE_MYDB
   int rc;
#endif
   int i=0, tm=train_mode, saved_uid = sdata->uid;
   float spaminess = DEFAULT_SPAMICITY;

   if(counthash(state->token_hash) <= 0) return 0;

   /* disable some configuration settings, 2008.06.28, SJ */

   memset(cfg->rbl_domain, 0, MAXVAL);
   memset(cfg->surbl_domain, 0, MAXVAL);
   cfg->penalize_images = 0;
   cfg->penalize_embed_images = 0;
   cfg->penalize_octet_stream = 0;


   if(cfg->group_type == GROUP_SHARED) sdata->uid = 0;


#ifndef HAVE_MYDB
   introduceTokens(sdata, state->token_hash); 
#endif

   for(i=0; i<rounds; i++){

      /* first, update the counters */

   #ifdef HAVE_MYDB
      my_walk_hash(cfg->mydbfile, sdata->mhash, is_spam, state->token_hash, tm);
   #else
      updateTokenCounters(sdata, is_spam, state->token_hash, T_TOE);
      updateMiscTable(sdata, is_spam, T_TOE);

      /* fix counters if it was a TUM training */

      if(tm == T_TUM){
         updateTokenCounters(sdata, is_spam, state->token_hash, T_TUM);
         updateMiscTable(sdata, is_spam, T_TUM);
      }
   #endif


      /* then, query the new spamicity value */

   #ifdef HAVE_MYDB
      rc = init_mydb(cfg->mydbfile, sdata);
      if(rc == 1) spaminess = bayes_file(sdata, state, cfg);
      close_mydb(sdata->mhash);
   #else
      spaminess = bayes_file(sdata, state, cfg);
   #endif

      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: training round: %d, spaminess: %0.4f", sdata->ttmpfile, i, spaminess);

      if(is_spam == 1 && spaminess > 0.99) break;
      if(is_spam == 0 && spaminess < 0.1) break;


      tm = T_TOE;
   }


   sdata->uid = saved_uid;

   return i+1;
}

