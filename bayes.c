/*
 * bayes.c, 2008.09.15, SJ
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
#include "config.h"

struct node *s_phrase_hash[MAXHASH], *shash[MAXHASH], *s_mix[MAXHASH];
struct node *B_hash[MAXHASH];
float n_phrases = 0;
float n_tokens = 0;
float n_mix = 0;


struct timezone tz;
struct timeval tv1, tv2;
qry QRY;


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

#ifdef HAVE_MYSQL
int assign_spaminess(MYSQL mysql, char *p, struct __config cfg, unsigned int uid){
#endif
#ifdef HAVE_SQLITE3
int assign_spaminess(sqlite3 *db, char *p, struct __config cfg, unsigned int uid){
#endif
#ifdef HAVE_MYDB
int assign_spaminess(struct mydb_node *mhash[], char *p, struct session_data *sdata, struct __config cfg, unsigned int uid){
#endif

   float spaminess=0;
   char t[MAX_TOKEN_LEN], *s;

   /* if we already have this token, 2006.03.13, SJ */

   if(findnode(s_mix, p))
      return 0;

#ifdef HAVE_MYSQL
   spaminess = SQL_QUERY(QRY, cfg.group_type, SQL_TOKEN_TABLE, p);
#endif
#ifdef HAVE_SQLITE3
   spaminess = SQL_QUERY(QRY, cfg.group_type, SQL_TOKEN_TABLE, p);
#endif
#ifdef HAVE_MYDB
   spaminess = mydbqry(mhash, p, sdata, cfg.rob_s, cfg.rob_x);
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
      spaminess = SQL_QUERY(QRY, cfg.group_type, SQL_TOKEN_TABLE, t);
   #endif
   #ifdef HAVE_SQLITE3
      spaminess = SQL_QUERY(QRY, cfg.group_type, SQL_TOKEN_TABLE, t);
   #endif
   #ifdef HAVE_MYDB
      spaminess = mydbqry(mhash, t, sdata, cfg.rob_s, cfg.rob_x);
   #endif
   }


   /* exclude unknown tokens, 2008.01.08, SJ */

   if(DEVIATION(spaminess) > 0.1){
      if(strchr(p, '+') || strchr(p, '*')){
         n_phrases += addnode(s_phrase_hash, p, spaminess, DEVIATION(spaminess));
         n_mix += addnode(s_mix, p, spaminess, DEVIATION(spaminess));
      }
      if(strchr(p, '+') == NULL){
         n_tokens += addnode(shash, p, spaminess, DEVIATION(spaminess));
         n_mix += addnode(s_mix, p, spaminess, DEVIATION(spaminess));
      }
   }

   return 0;
}


/*
 * calc_score
 */

double calc_score(struct node *xhash[], struct __config cfg){
   return calc_score_chi2(xhash, cfg);
}


#ifdef HAVE_MYSQL
int walk_hash(MYSQL mysql, struct node *xhash[], struct __config cfg){
#endif
#ifdef HAVE_SQLITE3
int walk_hash(sqlite3 *db, struct node *xhash[], struct __config cfg){
#endif
#ifdef HAVE_MYDB
int walk_hash(struct mydb_node *mhash[], struct node *xhash[], struct session_data *sdata, struct __config cfg){
#endif

   int i, n=0;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

      #ifdef HAVE_MYSQL
         assign_spaminess(mysql, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_SQLITE3
         assign_spaminess(db, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_MYDB
         assign_spaminess(mhash, p->str, sdata, cfg, QRY.uid);
      #endif

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

      //if(state.message_state != MSG_BODY && strncmp(buf, tumbuf, strlen(tumbuf)) == 0){
      if(strncmp(buf, tumbuf, strlen(tumbuf)) == 0){
         //printf("tum trained message\n");
         state.train_mode = T_TUM;
      }
   }

   fclose(f);

   return state;
}


/*
 * evaulate tokens
 */

#ifdef HAVE_MYSQL
double eval_tokens(MYSQL mysql, struct session_data *sdata, struct __config cfg, struct _state state){
#endif
#ifdef HAVE_SQLITE3
double eval_tokens(sqlite3 *db, struct session_data *sdata, struct __config cfg, struct _state state){
#endif
#ifdef HAVE_MYDB
double eval_tokens(struct mydb_node *mhash[], struct session_data *sdata, struct __config cfg, struct _state state){
#endif

   unsigned long n = 0;
   struct _token *p, *q;
   float spaminess, spaminess2;
   int found_on_rbl=0, has_embed_image=0, surbl_match=0;
#ifdef HAVE_RBL
   struct url *url;
   char surbl_token[MAX_TOKEN_LEN];
   int i, j;
#endif

   if(!state.first)
      return DEFAULT_SPAMICITY;

   surbl_match = 0;
   spaminess = spaminess2 = DEFAULT_SPAMICITY;

   p = state.first;

   inithash(shash);
   inithash(s_phrase_hash);
   inithash(s_mix);

   inithash(B_hash);

   while(p != NULL){
      q = p->r;

      /* add to URL hash, 2006.06.23, SJ */

      if(strncmp(p->str, "URL*", 4) == 0){
      #ifdef HAVE_MYSQL
         assign_spaminess(mysql, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_SQLITE3
         assign_spaminess(db, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_MYDB
         assign_spaminess(mhash, p->str, sdata, cfg, QRY.uid);
      #endif

         addnode(B_hash, p->str, 0, 0);
      }

      /* 2007.06.06, SJ */

      if(cfg.use_pairs == 1 && (strchr(p->str, '+') || strchr(p->str, '*')) ){
      #ifdef HAVE_MYSQL
         assign_spaminess(mysql, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_SQLITE3
         assign_spaminess(db, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_MYDB
         assign_spaminess(mhash, p->str, sdata, cfg, QRY.uid);
      #endif
      }

      else addnode(B_hash, p->str, 0, 0);

      if(n > 0){

         /* we may penalize embedded images, 2007.01.03, SJ */

         if(cfg.penalize_embed_images == 1 && strcmp(p->str, "src+cid") == 0){
            spaminess = REAL_SPAM_TOKEN_PROBABILITY;
            n_phrases += addnode(s_phrase_hash, "EMBED*", spaminess, DEVIATION(spaminess));
            n_tokens += addnode(shash, "EMBED*", spaminess, DEVIATION(spaminess));
            n_mix += addnode(s_mix, "EMBED*", spaminess, DEVIATION(spaminess));
            has_embed_image = 1;
         }

      }

      n++;

      p = q;
   }


   /* add a spammy token if we got a binary, eg. PDF attachment, 2007.07.02, SJ */

   if(cfg.penalize_octet_stream == 1 && (attachment_by_type(state, "application/octet-stream") == 1 || attachment_by_type(state, "application/pdf") == 1
       || attachment_by_type(state, "application/vnd.ms-excel") == 1
       || attachment_by_type(state, "application/msword") == 1
       || attachment_by_type(state, "application/rtf") == 1
       || attachment_by_type(state, "application/x-zip-compressed") == 1)
   ){
       spaminess = REAL_SPAM_TOKEN_PROBABILITY;
       n_phrases += addnode(s_phrase_hash, "OCTET_STREAM*", spaminess, DEVIATION(spaminess));
       n_tokens += addnode(shash, "OCTET_STREAM*", spaminess, DEVIATION(spaminess));
       n_mix += addnode(s_mix, "OCTET_STREAM*", spaminess, DEVIATION(spaminess));
   }

   /* add penalty for images, 2007.07.02, SJ */

   if(cfg.penalize_images == 1 && attachment_by_type(state, "image/") == 1){
       spaminess = REAL_SPAM_TOKEN_PROBABILITY;
       n_phrases += addnode(s_phrase_hash, "IMAGE*", spaminess, DEVIATION(spaminess));
       n_tokens += addnode(shash, "IMAGE*", spaminess, DEVIATION(spaminess));
       n_mix += addnode(s_mix, "IMAGE*", spaminess, DEVIATION(spaminess));
   }


   if(state.n_subject_token == 0){
      spaminess = REAL_SPAM_TOKEN_PROBABILITY;
      n_phrases += addnode(s_phrase_hash, "NO_SUBJECT*", spaminess, DEVIATION(spaminess));
      n_tokens += addnode(shash, "NO_SUBJECT*", spaminess, DEVIATION(spaminess));
      n_mix += addnode(s_mix, "NO_SUBJECT*", spaminess, DEVIATION(spaminess));
   }

   /* add the From line, 2007.06.16, SJ */

#ifdef HAVE_MYSQL
   assign_spaminess(mysql, state.from, cfg, QRY.uid);
#endif
#ifdef HAVE_SQLITE3
   assign_spaminess(db, state.from, cfg, QRY.uid);
#endif
#ifdef HAVE_MYDB
   assign_spaminess(mhash, state.from, sdata, cfg, QRY.uid);
#endif

   addnode(B_hash, state.from, 0, 0);

#ifdef HAVE_XFORWARD
   if(sdata->unknown_client == 1 && QRY.ham_msg > NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED){
      spaminess = REAL_SPAM_TOKEN_PROBABILITY;
      n_phrases += addnode(s_phrase_hash, "UNKNOWN_CLIENT*", spaminess, DEVIATION(spaminess));
      n_tokens += addnode(shash, "UNKNOWN_CLIENT*", spaminess, DEVIATION(spaminess));
      n_mix += addnode(s_mix, "UNKNOWN_CLIENT*", spaminess, DEVIATION(spaminess));
   }
#endif

   /* redesigned spaminess calculation, 2007.08.28, SJ */

   if(cfg.use_pairs == 1){
      spaminess = calc_score(s_phrase_hash, cfg);

      if(cfg.debug == 1)
         fprintf(stderr, "phrase: %.4f\n", spaminess);

      if(spaminess >= cfg.spam_overall_limit || spaminess <= cfg.max_ham_spamicity)
         goto END_OF_EVALUATION;

   }

   if(cfg.use_single_tokens == 1){
   #ifdef HAVE_MYSQL
      n_tokens += walk_hash(mysql, B_hash, cfg);
   #endif
   #ifdef HAVE_SQLITE3
      n_tokens += walk_hash(db, B_hash, cfg);
   #endif
   #ifdef HAVE_MYDB
      n_tokens += walk_hash(mhash, B_hash, sdata, cfg);
   #endif

      /* consult blacklists about the IPv4 address connecting to us */

   #ifdef HAVE_RBL
      if(strlen(cfg.rbl_domain) > 3){
         gettimeofday(&tv1, &tz);
         found_on_rbl = rbl_list_check(cfg.rbl_domain, state.ip, cfg.debug);
         gettimeofday(&tv2, &tz);

         if(cfg.debug == 1) fprintf(stderr, "rbl check took %ld ms\n", tvdiff(tv2, tv1)/1000);
         if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: rbl check took %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

         for(i=0; i<found_on_rbl; i++){
            snprintf(surbl_token, MAX_TOKEN_LEN-1, "RBL%d*%s", i, state.ip);
            n_phrases += addnode(s_phrase_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            n_tokens += addnode(shash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            n_mix += addnode(s_mix, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
         }
      }
   #endif

      /* consult URL blacklists */

   #ifdef HAVE_RBL
      if(state.urls){
         url = state.urls;

         while(url){
            gettimeofday(&tv1, &tz);
            i = rbl_list_check(cfg.surbl_domain, url->url_str+4, cfg.debug);
            gettimeofday(&tv2, &tz);

            if(cfg.debug == 1) fprintf(stderr, "surbl check for %s (%d) took %ld ms\n", url->url_str+4, i, tvdiff(tv2, tv1)/1000);
            if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: surbl check took %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

            surbl_match += i;

            for(j=0; j<i; j++){
               snprintf(surbl_token, MAX_TOKEN_LEN-1, "SURBL%d*%s", j, url->url_str+4);
               n_phrases += addnode(s_phrase_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
               n_tokens += addnode(shash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
               n_mix += addnode(s_mix, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            }

            url = url->r;
         }
      }
   #endif


      if(cfg.use_pairs == 1){
         spaminess = calc_score(s_mix, cfg);
         if(cfg.debug == 1)
            fprintf(stderr, "mix: %.4f\n", spaminess);

         if(spaminess >= cfg.spam_overall_limit || spaminess <= cfg.max_ham_spamicity)
            goto END_OF_EVALUATION;
      }


   }


END_OF_EVALUATION:

   clearhash(shash);
   clearhash(s_phrase_hash);
   clearhash(s_mix);
   clearhash(B_hash);



   /* if the message is unsure, try to determine if it's a spam, 2008.01.09, SJ */

   if(spaminess > cfg.max_ham_spamicity && spaminess < cfg.spam_overall_limit)
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
struct c_res bayes_file(MYSQL mysql, struct _state state, struct session_data sdata, struct __config cfg){
#endif
#ifdef HAVE_SQLITE3
struct c_res bayes_file(sqlite3 *db, struct _state state, struct session_data sdata, struct __config cfg){
#endif
#ifdef HAVE_MYDB
struct c_res bayes_file(struct mydb_node *mhash[], struct _state state, struct session_data sdata, struct __config cfg){
#endif

   char buf[MAXBUFSIZE], *p;
   float ham_from=0, spam_from=0;
   struct c_res result;

#ifdef HAVE_MYSQL
   struct te TE;
#endif
#ifdef HAVE_MYDB
   struct mydb_node *Q;
#endif

   result.spaminess = DEFAULT_SPAMICITY;
   result.ham_msg = 0;
   result.spam_msg = 0;


   /* init query structure */

#ifdef HAVE_MYSQL
   QRY.mysql = mysql;
#endif
#ifdef HAVE_SQLITE3
   QRY.db = db;
#endif
   QRY.uid = sdata.uid;
   QRY.sockfd = -1;
   QRY.ham_msg = 0;
   QRY.spam_msg = 0;
   QRY.rob_s = cfg.rob_s;
   QRY.rob_x = cfg.rob_x;


   p = strrchr(sdata.ttmpfile, '/');
   if(p)
      p++;
   else
      p = sdata.ttmpfile;


   /* evaluate the blackhole result, 2006.10.02, SJ */

#ifdef HAVE_BLACKHOLE
   if(strlen(cfg.blackhole_path) > 3 && blackness(cfg.blackhole_path, state.ip, cfg) > 100){
      syslog(LOG_PRIORITY, "%s: found %s on our blackhole", p, state.ip);

      result.spaminess = cfg.spaminess_of_blackholed_mail;
      return result;
   }
#endif

   /* switch to shared group if we are using mydb, 2007.10.09, SJ */

#ifdef HAVE_MYDB
   cfg.group_type = GROUP_SHARED;
#endif
#ifdef HAVE_SQLITE3
   cfg.group_type = GROUP_SHARED;
#endif

   /* fix uid and sql statement if this is a shared group */

   if(cfg.group_type == GROUP_SHARED){
      QRY.uid = 0;
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);
   }

   /* fix sql statement if we use a merged group */
   if(cfg.group_type == GROUP_MERGED)
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0 OR uid=%ld", SQL_MISC_TABLE, QRY.uid);

   if(cfg.debug == 1)
      fprintf(stderr, "uid: %ld\n", QRY.uid);

   /*
    * select the number of ham and spam messages, and return error if less than 1
    */

#ifdef HAVE_MYSQL
   TE = get_ham_spam(mysql, buf);
   QRY.ham_msg = TE.nham;
   QRY.spam_msg = TE.nspam;
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      while(sqlite3_step(pStmt) == SQLITE_ROW){
         QRY.ham_msg += sqlite3_column_int(pStmt, 0);
         QRY.spam_msg += sqlite3_column_int(pStmt, 1);
      }
   }
   sqlite3_finalize(pStmt);
#endif

#ifdef HAVE_MYDB
   QRY.ham_msg = sdata.Nham;
   QRY.spam_msg = sdata.Nspam;
#endif

   result.ham_msg = QRY.ham_msg;
   result.spam_msg = QRY.spam_msg;

   if((QRY.ham_msg + QRY.spam_msg == 0) && cfg.initial_1000_learning == 0){
      syslog(LOG_PRIORITY, "%s: %s", p, ERR_SQL_DATA);
      return result;
   }

   if(cfg.debug == 1)
      fprintf(stderr, "nham: %.0f, nspam: %.0f\n", QRY.ham_msg, QRY.spam_msg);

   /*
    * auto whitelist test, 2007.06.21, SJ
    * it may be better to only count the users own experiment with this sender...
    * A further note: this way we will not TUM train the message if it's whitelisted
    */

   if(cfg.enable_auto_white_list == 1){

   #ifdef HAVE_MYSQL
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND (uid=0 OR uid=%ld)", SQL_TOKEN_TABLE, APHash(state.from), QRY.uid);
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
      Q = findmydb_node(mhash, APHash(state.from));
      if(Q){
         ham_from = Q->nham;
         spam_from = Q->nspam;
      }
   #endif

      if(cfg.debug == 1)
         fprintf(stderr, "from: %.0f, %.0f\n", ham_from, spam_from);

      if(ham_from > NUMBER_OF_GOOD_FROM && spam_from == 0){
         result.spaminess = REAL_HAM_TOKEN_PROBABILITY;
         return result;
      }
   }


/* Query cache support */

#ifdef HAVE_QCACHE
   QRY.sockfd = qcache_socket(cfg.qcache_addr, cfg.qcache_port, cfg.qcache_socket);
   if(QRY.sockfd == -1)
      return result;
#endif


   /* evaluate the tokens */

#ifdef HAVE_MYSQL
   result.spaminess = eval_tokens(mysql, &sdata, cfg, state);
#endif
#ifdef HAVE_SQLITE3
   result.spaminess = eval_tokens(db, &sdata, cfg, state);
#endif
#ifdef HAVE_MYDB
   result.spaminess = eval_tokens(mhash, &sdata, cfg, state);
#endif

   return result;
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
   struct mydb_node *mhash2[MAX_MYDB_HASH];
#endif
   int i, tm=train_mode;
   char buf[SMALLBUFSIZE];
   struct c_res result;

   if(state.first == NULL) return 0;

   /* disable some configuration settings, 2008.06.28, SJ */

   memset(cfg.rbl_domain, 0, MAXVAL);
   memset(cfg.surbl_domain, 0, MAXVAL);
   cfg.penalize_images = 0;
   cfg.penalize_embed_images = 0;
   cfg.penalize_octet_stream = 0;

   result.spaminess = DEFAULT_SPAMICITY;

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

         result = bayes_file(mysql, state, sdata, cfg);
      #endif
      #ifdef HAVE_SQLITE3
         sqlite3_exec(db, buf, NULL, NULL, &err);

         result = bayes_file(db, state, sdata, cfg);
      #endif
      }

   #ifdef HAVE_MYDB
      rc = init_mydb(cfg.mydbfile, mhash2, &sdata);
      if(rc == 1){
         result = bayes_file(mhash2, state, sdata, cfg);
      }
      close_mydb(mhash2);
   #endif

      if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "training round: %d, spaminess: %0.4f", i, result.spaminess);

      if(is_spam == 1 && result.spaminess > 0.99) return i;
      if(is_spam == 0 && result.spaminess < 0.1) return i;


      tm = T_TOE;
   }

   return 0;
}

