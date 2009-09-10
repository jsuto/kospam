/*
 * bayes.c, 2009.09.10, SJ
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
#include "sql.h"
#include "bayes.h"
#include "config.h"
#include "buffer.h"


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

   update_hash(sdata->mysql, query->data, state->token_hash, cfg);
#endif
#ifdef HAVE_SQLITE3
   buffer_cat(query, ")");
   update_hash(sdata->db, query->data, state->token_hash, cfg);
#endif

   buffer_destroy(query);

   calcnode(state->token_hash, sdata->Nham, sdata->Nspam, cfg);

   return 1;
}


/*
 * evaulate tokens
 */

double eval_tokens(struct session_data *sdata, struct _state *state, struct __config *cfg){
   float spaminess=DEFAULT_SPAMICITY;
   int has_embed_image=0, found_on_rbl=0, surbl_match=0;


   if(cfg->penalize_embed_images == 1 && findnode(state->token_hash, "src+cid")){
      addnode(state->token_hash, "EMBED*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      has_embed_image = 1;
   }


   /* calculate spaminess based on the token pairs and other special tokens */

   qry_spaminess(sdata, state, 1, cfg);

   /* apply some penalties, 2009.06.01, SJ */
   add_penalties(sdata, state, cfg);

   spaminess = calc_score_chi2(state->token_hash, cfg);

   if(cfg->debug == 1) printf("phrase: %.4f\n", spaminess);

   if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity) goto END_OF_EVALUATION;

   /* query the single tokens, then use the 'mix' for calculation */

   qry_spaminess(sdata, state, 0, cfg);
   spaminess = calc_score_chi2(state->token_hash, cfg);
   if(cfg->debug == 1) printf("mix: %.4f\n", spaminess);
   if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity) goto END_OF_EVALUATION;


   /* if we are still unsure, consult blacklists */
 
#ifdef HAVE_RBL
   check_lists(sdata, state, &found_on_rbl, &surbl_match, cfg);
#endif

   spaminess = calc_score_chi2(state->token_hash, cfg);
   if(cfg->debug == 1) printf("mix after blacklists: %.4f\n", spaminess);



END_OF_EVALUATION:


   /* if the message is unsure, try to determine if it's a spam, 2008.01.09, SJ */

   if(spaminess > cfg->max_ham_spamicity && spaminess < cfg->spam_overall_limit)
      spaminess = apply_fixes(spaminess, found_on_rbl, surbl_match, has_embed_image, state->base64_text, state->c_shit, state->l_shit, cfg);


   /* fix spaminess value if we have to */

   if(spaminess < 0) spaminess = REAL_HAM_TOKEN_PROBABILITY;
   if(spaminess > 1) spaminess = REAL_SPAM_TOKEN_PROBABILITY;

   return spaminess;
}


/*
 * Bayesian result of the file
 */

float bayes_file(struct session_data *sdata, struct _state *state, struct __config *cfg){
   char buf[MAXBUFSIZE], *p;
   float ham_from=0, spam_from=0;
   float spaminess = DEFAULT_SPAMICITY;

#ifdef HAVE_MYSQL
   struct te TE;
#endif
#ifdef HAVE_MYDB
   struct mydb_node *Q;
#endif


   p = strrchr(sdata->ttmpfile, '/');
   if(p)
      p++;
   else
      p = sdata->ttmpfile;


#ifdef HAVE_MYDB
   cfg->group_type = GROUP_SHARED;
#endif
#ifdef HAVE_SQLITE3
   cfg->group_type = GROUP_SHARED;
#endif


   /* assemble sql statement group */

   if(cfg->group_type == GROUP_SHARED)
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);
   else
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0 OR uid=%ld", SQL_MISC_TABLE, sdata->uid);

   if(cfg->debug == 1)
      printf("username: %s, uid: %ld\n", sdata->name, sdata->uid);

   /*
    * select the number of ham and spam messages, and return error if less than 1
    */

#ifdef HAVE_MYSQL
   TE = get_ham_spam(sdata->mysql, buf);
   sdata->Nham = TE.nham;
   sdata->Nspam = TE.nspam;
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      while(sqlite3_step(pStmt) == SQLITE_ROW){
         sdata->Nham += sqlite3_column_int(pStmt, 0);
         sdata->Nspam += sqlite3_column_int(pStmt, 1);
      }
   }
   sqlite3_finalize(pStmt);
#endif


   if((sdata->Nham + sdata->Nspam == 0) && cfg->initial_1000_learning == 0){
      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: %s", p, ERR_SQL_DATA);
      return DEFAULT_SPAMICITY;
   }

   if(cfg->debug == 1)
      printf("nham: %.0f, nspam: %.0f\n", sdata->Nham, sdata->Nspam);

   /*
    * auto whitelist test, 2007.06.21, SJ
    * it may be better to only count the users own experiment with this sender...
    * A further note: this way we will not TUM train the message if it's whitelisted
    */

   if(cfg->enable_auto_white_list == 1){

   #ifdef HAVE_MYSQL
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND (uid=0 OR uid=%ld)", SQL_TOKEN_TABLE, APHash(state->from), sdata->uid);
      TE = get_ham_spam(sdata->mysql, buf);
      ham_from = TE.nham;
      spam_from = TE.nspam;
   #endif
   #ifdef HAVE_SQLITE3
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu", SQL_TOKEN_TABLE, APHash(state->from));
      if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
         if(sqlite3_step(pStmt) == SQLITE_ROW){
            ham_from = sqlite3_column_int(pStmt, 0);
            spam_from = sqlite3_column_int(pStmt, 1);
         }
      }
      sqlite3_finalize(pStmt);
   #endif
   #ifdef HAVE_MYDB
      Q = findmydb_node(sdata->mhash, APHash(state->from));
      if(Q){
         ham_from = Q->nham;
         spam_from = Q->nspam;
      }
   #endif

      if(cfg->debug == 1)
         printf("from: %.0f, %.0f\n", ham_from, spam_from);

      if(ham_from > NUMBER_OF_GOOD_FROM && spam_from == 0){
         if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: sender is statistically whitelisted", sdata->ttmpfile);

         /* we query the spaminess of the token pairs in order to have their timestamp updated */
         qry_spaminess(sdata, state, 1, cfg);

         return REAL_HAM_TOKEN_PROBABILITY;
      }
   }


   int saved_uid = sdata->uid;

   if(cfg->group_type == GROUP_SHARED) sdata->uid = 0;

   /* evaluate the tokens */
   spaminess = eval_tokens(sdata, state, cfg);

   /* restore saved uid */
   if(cfg->group_type == GROUP_SHARED) sdata->uid = saved_uid;

   return spaminess;
}


/*
 * train with this message
 */

int train_message(struct session_data *sdata, struct _state *state, int rounds, int is_spam, int train_mode, struct __config *cfg){
#ifdef HAVE_SQLITE3
   char *err=NULL;
#endif
#ifdef HAVE_MYDB
   int rc;
#endif
   int i=0, tm=train_mode, saved_uid = sdata->uid;
   char buf[SMALLBUFSIZE];
   float spaminess = DEFAULT_SPAMICITY;

   if(counthash(state->token_hash) <= 0) return 0;

   /* disable some configuration settings, 2008.06.28, SJ */

   memset(cfg->rbl_domain, 0, MAXVAL);
   memset(cfg->surbl_domain, 0, MAXVAL);
   cfg->penalize_images = 0;
   cfg->penalize_embed_images = 0;
   cfg->penalize_octet_stream = 0;


   if(cfg->group_type == GROUP_SHARED) sdata->uid = 0;

   for(i=0; i<rounds; i++){
   #ifdef HAVE_MYSQL
      my_walk_hash(sdata->mysql, is_spam, sdata->uid, state->token_hash, tm);
   #endif
   #ifdef HAVE_SQLITE3
      my_walk_hash(sdata->db, is_spam, state->token_hash, tm);
   #endif
   #ifdef HAVE_MYDB
      my_walk_hash(cfg->mydbfile, sdata->mhash, is_spam, state->token_hash, tm);
   #else

      /* update the t_misc table */

      if(is_spam == 1)
         snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->uid);
      else
         snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->uid);
   #endif

   #ifdef HAVE_MYSQL
      mysql_real_query(&(sdata->mysql), buf, strlen(buf));
   #endif
   #ifdef HAVE_SQLITE3
      sqlite3_exec(sdata->db, buf, NULL, NULL, &err);
   #endif

      /* fix the message counters if it was a TUM training */

      if(tm == T_TUM){
         if(is_spam == 1)
            snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham-1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->uid);
         else
            snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam-1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->uid);

      #ifdef HAVE_MYSQL
         mysql_real_query(&(sdata->mysql), buf, strlen(buf));
      #endif
      #ifdef HAVE_SQLITE3
         sqlite3_exec(sdata->db, buf, NULL, NULL, &err);
      #endif
      }


      /* break the training loop in case of TOE mode, 2009.09.10, SJ */
      if(train_mode == T_TOE) break;


      /* query the new spamicity value in this round */

   #ifdef HAVE_MYSQL
      spaminess = bayes_file(sdata, state, cfg);
   #endif
   #ifdef HAVE_SQLITE3
      spaminess = bayes_file(sdata, state, cfg);
   #endif
   #ifdef HAVE_MYDB
      rc = init_mydb(cfg->mydbfile, sdata);
      if(rc == 1) spaminess = bayes_file(sdata, state, cfg);
      close_mydb(sdata->mhash);
   #endif

      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: training round: %d, spaminess: %0.4f", sdata->ttmpfile, i, spaminess);

      if(is_spam == 1 && spaminess > 0.99) break;
      if(is_spam == 0 && spaminess < 0.1) break;


      tm = T_TOE;
   }


   sdata->uid = saved_uid;

   return i;
}

