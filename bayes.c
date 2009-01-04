/*
 * bayes.c, 2009.01.04, SJ
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
 * assign spaminess value to token
 */

int assign_spaminess(struct session_data *sdata, char *p, struct __config *cfg, struct node *s_phrase_hash[], struct node *s_mix[]){
   float spaminess=0;
   char t[MAX_TOKEN_LEN], *s;

   /* if we already have this token, 2006.03.13, SJ */

   if(findnode(s_mix, p))
      return 0;

#ifdef HAVE_MYSQL
   spaminess = SQL_QUERY(sdata, p, cfg);
#endif
#ifdef HAVE_SQLITE3
   spaminess = SQL_QUERY(sdata, p, cfg);
#endif
#ifdef HAVE_MYDB
   spaminess = mydbqry(sdata, p, cfg);
#endif

   /* if it was at the Subject: header line, let's try it if it were not in the Subject line, 2006.05.03, SJ */

   if(spaminess < DEFAULT_SPAMICITY_HIGH && spaminess > DEFAULT_SPAMICITY_LOW && strncasecmp(p, "Subject*", 8) == 0){
      memset(t, 0, MAX_TOKEN_LEN);

      s = strchr(p, '+');
      if(s){
         *s = '\0';
         snprintf(t, MAX_TOKEN_LEN-1, "%s+%s", p+8, s+1+8);
      }
      else
         strncpy(t, p+8, MAX_TOKEN_LEN-1);

   #ifdef HAVE_MYSQL
      spaminess = SQL_QUERY(sdata, t, cfg);
   #endif
   #ifdef HAVE_SQLITE3
      spaminess = SQL_QUERY(sdata, t, cfg);
   #endif
   #ifdef HAVE_MYDB
      spaminess = mydbqry(sdata, t, cfg);
   #endif
   }


   /* exclude unknown tokens, 2008.01.08, SJ */

   if(DEVIATION(spaminess) > 0.1){
      if(strchr(p, '+') || strchr(p, '*')){
         addnode(s_phrase_hash, p, spaminess, DEVIATION(spaminess));
         addnode(s_mix, p, spaminess, DEVIATION(spaminess));
      }
      if(strchr(p, '+') == NULL)
         addnode(s_mix, p, spaminess, DEVIATION(spaminess));
   }

   return 0;
}


/*
 * calc_score
 */

inline double calc_score(struct node *xhash[], struct __config *cfg){
   return calc_score_chi2(xhash, cfg);
}


int walk_hash(struct session_data *sdata, struct node *xhash[], struct __config *cfg, struct node *s_phrase_hash[], struct node *s_mix[]){
   int i, n=0;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

         assign_spaminess(sdata, p->str, cfg, s_phrase_hash, s_mix);

         n++;

         q = q->r;
      }
   }

   return n;
}


/*
 * parse the message into tokens and return the pointer
 */

struct _state parse_message(char *spamfile, struct session_data sdata, struct __config cfg){
   FILE *f;
   char buf[MAXBUFSIZE], tumbuf[SMALLBUFSIZE];
   struct _state state;

   init_state(&state);

   f = fopen(spamfile, "r");
   if(!f){
      syslog(LOG_PRIORITY, "%s: cannot open", spamfile);
      return state;
   }

   snprintf(tumbuf, SMALLBUFSIZE-1, "%sTUM", cfg.clapf_header_field);

   while(fgets(buf, MAXBUFSIZE-1, f)){
      parse(buf, &state, &sdata, cfg);

      if(strncmp(buf, tumbuf, strlen(tumbuf)) == 0){
         state.train_mode = T_TUM;
      }
   }

   fclose(f);

   return state;
}


/*
 * evaulate tokens
 */

double eval_tokens(struct session_data *sdata, struct __config *cfg, struct _state state){
   unsigned long n = 0;
   struct _token *p, *q;
   float spaminess, spaminess2;
   int has_embed_image=0;
   struct node *s_phrase_hash[MAXHASH], *s_mix[MAXHASH];
   struct node *B_hash[MAXHASH];
#ifdef HAVE_RBL
   int found_on_rbl=0, surbl_match=0;
#endif

   if(!state.first)
      return DEFAULT_SPAMICITY;

   spaminess = spaminess2 = DEFAULT_SPAMICITY;

   p = state.first;

   inithash(s_phrase_hash);
   inithash(s_mix);

   inithash(B_hash);

   while(p != NULL){
      q = p->r;

      /* add to URL hash, 2006.06.23, SJ */

      if(strncmp(p->str, "URL*", 4) == 0){
         assign_spaminess(sdata, p->str, cfg, s_phrase_hash, s_mix);
         addnode(B_hash, p->str, 0, 0);
      }

      /* 2007.06.06, SJ */

      if(cfg->use_pairs == 1 && (strchr(p->str, '+') || strchr(p->str, '*')) ){
         assign_spaminess(sdata, p->str, cfg, s_phrase_hash, s_mix);
      }

      else addnode(B_hash, p->str, 0, 0);

      if(n > 0){

         /* we may penalize embedded images, 2007.01.03, SJ */

         if(cfg->penalize_embed_images == 1 && strcmp(p->str, "src+cid") == 0){
            append_to_hash_tables(s_phrase_hash, s_mix, "EMBED*", REAL_SPAM_TOKEN_PROBABILITY);
            has_embed_image = 1;
         }

      }

      n++;

      p = q;
   }


   /* apply some penalties, 2009.01.04, SJ */
   add_penalties(sdata, s_phrase_hash, s_mix, state, cfg);


   /* add the From line, 2007.06.16, SJ */

   assign_spaminess(sdata, state.from, cfg, s_phrase_hash, s_mix);
   addnode(B_hash, state.from, 0, 0);


   /* redesigned spaminess calculation, 2007.08.28, SJ */

   if(cfg->use_pairs == 1){
      spaminess = calc_score(s_phrase_hash, cfg);

      if(cfg->debug == 1)
         fprintf(stderr, "phrase: %.4f\n", spaminess);

      if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity)
         goto END_OF_EVALUATION;

   }

   if(cfg->use_single_tokens == 1){
      walk_hash(sdata, B_hash, cfg, s_phrase_hash, s_mix);

      /* consult blacklists */
   #ifdef HAVE_RBL
      check_lists(sdata, &state, s_phrase_hash, s_mix, &found_on_rbl, &surbl_match, cfg);
   #endif


      if(cfg->use_pairs == 1){
         spaminess = calc_score(s_mix, cfg);
         if(cfg->debug == 1)
            fprintf(stderr, "mix: %.4f\n", spaminess);

         if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity)
            goto END_OF_EVALUATION;
      }


   }


END_OF_EVALUATION:

   clearhash(s_phrase_hash);
   clearhash(s_mix);
   clearhash(B_hash);



   /* if the message is unsure, try to determine if it's a spam, 2008.01.09, SJ */

   if(spaminess > cfg->max_ham_spamicity && spaminess < cfg->spam_overall_limit)
      spaminess = apply_fixes(spaminess, found_on_rbl, surbl_match, has_embed_image, state.base64_text, state.c_shit, state.l_shit, cfg);


   /* fix spaminess value if we have to */

   if(spaminess < 0) spaminess = REAL_HAM_TOKEN_PROBABILITY;
   if(spaminess > 1) spaminess = REAL_SPAM_TOKEN_PROBABILITY;

   return spaminess;
}


/*
 * Bayesian result of the file
 */

#ifdef HAVE_MYSQL
float bayes_file(MYSQL mysql, struct _state state, struct session_data *sdata, struct __config *cfg){
#endif
#ifdef HAVE_SQLITE3
float bayes_file(sqlite3 *db, struct _state state, struct session_data *sdata, struct __config *cfg){
#endif
#ifdef HAVE_MYDB
float bayes_file(struct _state state, struct session_data *sdata, struct __config *cfg){
#endif

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


   /* evaluate the blackhole result, 2006.10.02, SJ */

#ifdef HAVE_BLACKHOLE
   if(strlen(cfg.blackhole_path) > 3 && blackness(cfg.blackhole_path, state.ip, cfg) > 100){
      syslog(LOG_PRIORITY, "%s: found %s on our blackhole", p, state.ip);

      return cfg->spaminess_of_blackholed_mail;
   }
#endif

#ifdef HAVE_MYSQL
   sdata->mysql = mysql;
#endif
#ifdef HAVE_MYDB
   cfg->group_type = GROUP_SHARED;
#endif
#ifdef HAVE_SQLITE3
   sdata->db = db;
   cfg->group_type = GROUP_SHARED;
#endif


   /* fix uid and sql statement if this is a shared group */

   if(cfg->group_type == GROUP_SHARED){
      sdata->uid = 0;
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);
   }

   /* fix sql statement if we use a merged group */
   if(cfg->group_type == GROUP_MERGED)
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0 OR uid=%ld", SQL_MISC_TABLE, sdata->uid);

   if(cfg->debug == 1)
      fprintf(stderr, "uid: %ld\n", sdata->uid);

   /*
    * select the number of ham and spam messages, and return error if less than 1
    */

#ifdef HAVE_MYSQL
   TE = get_ham_spam(mysql, buf);
   sdata->Nham = TE.nham;
   sdata->Nspam = TE.nspam;
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
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
      fprintf(stderr, "nham: %.0f, nspam: %.0f\n", sdata->Nham, sdata->Nspam);

   /*
    * auto whitelist test, 2007.06.21, SJ
    * it may be better to only count the users own experiment with this sender...
    * A further note: this way we will not TUM train the message if it's whitelisted
    */

   if(cfg->enable_auto_white_list == 1){

   #ifdef HAVE_MYSQL
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND (uid=0 OR uid=%ld)", SQL_TOKEN_TABLE, APHash(state.from), sdata->uid);
      TE = get_ham_spam(mysql, buf);
      ham_from = TE.nham;
      spam_from = TE.nspam;
   #endif
   #ifdef HAVE_SQLITE3
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu", SQL_TOKEN_TABLE, APHash(state.from));
      if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
         if(sqlite3_step(pStmt) == SQLITE_ROW){
            ham_from = sqlite3_column_int(pStmt, 0);
            spam_from = sqlite3_column_int(pStmt, 1);
         }
      }
      sqlite3_finalize(pStmt);
   #endif
   #ifdef HAVE_MYDB
      Q = findmydb_node(sdata->mhash, APHash(state.from));
      if(Q){
         ham_from = Q->nham;
         spam_from = Q->nspam;
      }
   #endif

      if(cfg->debug == 1)
         fprintf(stderr, "from: %.0f, %.0f\n", ham_from, spam_from);

      if(ham_from > NUMBER_OF_GOOD_FROM && spam_from == 0)
         return REAL_HAM_TOKEN_PROBABILITY;
   }



   /* evaluate the tokens */
   spaminess = eval_tokens(sdata, cfg, state);

   return spaminess;
}


/*
 * train with this message
 */

#ifdef HAVE_MYSQL
int train_message(MYSQL mysql, struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg){
#endif
#ifdef HAVE_SQLITE3
int train_message(sqlite3 *db, struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg){
   char *err=NULL;
#endif
#ifdef HAVE_MYDB
int train_message(char *mydbfile, struct mydb_node *mhash[], struct session_data sdata, struct _state state, int rounds, int is_spam, int train_mode, struct __config cfg){
   int rc;
   //struct mydb_node *mhash2[MAX_MYDB_HASH];
#endif
   int i, tm=train_mode;
   char buf[SMALLBUFSIZE];
   float spaminess = DEFAULT_SPAMICITY;

   if(state.first == NULL) return 0;

   /* disable some configuration settings, 2008.06.28, SJ */

   memset(cfg.rbl_domain, 0, MAXVAL);
   memset(cfg.surbl_domain, 0, MAXVAL);
   cfg.penalize_images = 0;
   cfg.penalize_embed_images = 0;
   cfg.penalize_octet_stream = 0;


   if(cfg.group_type == 0) sdata.uid = 0;

   for(i=1; i<=rounds; i++){
   #ifdef HAVE_MYSQL
      my_walk_hash(mysql, -1, is_spam, SQL_TOKEN_TABLE, sdata.uid, state.first, tm);
   #endif
   #ifdef HAVE_SQLITE3
      my_walk_hash(db, is_spam, SQL_TOKEN_TABLE, state.first, tm);
   #endif
   #ifdef HAVE_MYDB
      my_walk_hash(mydbfile, mhash, is_spam, state.first, tm);
   #else

      /* update the t_misc table */

      if(is_spam == 1)
         snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata.uid);
      else
         snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata.uid);
   #endif

   #ifdef HAVE_MYSQL
      mysql_real_query(&mysql, buf, strlen(buf));
   #endif
   #ifdef HAVE_SQLITE3
      sqlite3_exec(db, buf, NULL, NULL, &err);
   #endif

      if(train_mode == T_TUM){
         if(is_spam == 1)
            snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham-1 WHERE uid=%ld", SQL_MISC_TABLE, sdata.uid);
         else
            snprintf(buf, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam-1 WHERE uid=%ld", SQL_MISC_TABLE, sdata.uid);

      #ifdef HAVE_MYSQL
         mysql_real_query(&mysql, buf, strlen(buf));

         spaminess = bayes_file(mysql, state, &sdata, &cfg);
      #endif
      #ifdef HAVE_SQLITE3
         sqlite3_exec(db, buf, NULL, NULL, &err);

         spaminess = bayes_file(db, state, &sdata, &cfg);
      #endif
      }

   #ifdef HAVE_MYDB
      /*rc = init_mydb(cfg.mydbfile, mhash2, &sdata);
      if(rc == 1){
         spaminess = bayes_file(mhash2, state, &sdata, &cfg);
      }
      close_mydb(mhash2);*/

      rc = init_mydb(cfg.mydbfile, sdata.mhash, &sdata);
      if(rc == 1) spaminess = bayes_file(state, &sdata, &cfg);
      close_mydb(sdata.mhash);
   #endif

      if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "training round: %d, spaminess: %0.4f", i, spaminess);

      if(is_spam == 1 && spaminess > 0.99) return i;
      if(is_spam == 0 && spaminess < 0.1) return i;


      tm = T_TOE;
   }

   return 0;
}

