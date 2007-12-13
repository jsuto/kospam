/*
 * bayes.c, 2007.11.21, SJ
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
#include "black.h"
#include "rbl.h"
#include "messages.h"
#include "sql.h"
#include "config.h"

struct node *s_phrase_hash[MAXHASH], *shash[MAXHASH];
struct node *B_hash[MAXHASH], *tumhash[MAXHASH];
float n_phrases = 0;
float n_tokens = 0;

int surbl_match = 0;
int has_embed_image = 0;


struct timezone tz;
struct timeval tv1, tv2;
qry QRY;


#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL_RES *res;
   MYSQL_ROW row;
   int mysql_conn = 0;

   int my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;

   //int my_walk_hash(qry QRY, int ham_or_spam, struct node *xhash[MAXHASH], int train_mode);
   int my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode);
#endif

#ifdef HAVE_MYDB
   #include "mydb.h"
   float mydbqry(struct mydb_node *Mhash[MAX_MYDB_HASH], char *p, float rob_s, float rob_x, struct node *qhash[MAXHASH]);
   int my_walk_hash(char *mydbfile, struct mydb_node *xhash[MAX_MYDB_HASH], int ham_or_spam, struct node *qhash[MAXHASH], int train_mode);
   int update_tokens(char *mydbfile, struct mydb_node *xhash[MAX_MYDB_HASH], struct node *qhash[MAXHASH]);
#endif


 
float SQL_QUERY(qry QRY, char *tokentable, char *token, struct node *xhash[MAXHASH]);



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
int assign_spaminess(char *p, struct __config cfg, unsigned int uid){
#endif

   float spaminess=0;
   char t[MAX_TOKEN_LEN], *s;

   /* if we already have this token, 2006.03.13, SJ */

   if(findnode(shash, p))
      return 0;

#ifdef HAVE_MYSQL
   spaminess = SQL_QUERY(QRY, SQL_TOKEN_TABLE, p, tumhash);
#endif
#ifdef HAVE_SQLITE3
   spaminess = SQL_QUERY(QRY, SQL_TOKEN_TABLE, p, tumhash);
#endif
#ifdef HAVE_MYDB
   spaminess = mydbqry(mhash, p, cfg.rob_s, cfg.rob_x, tumhash);
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
      spaminess = SQL_QUERY(QRY, SQL_TOKEN_TABLE, t, tumhash);
   #endif
   #ifdef HAVE_SQLITE3
      spaminess = SQL_QUERY(QRY, SQL_TOKEN_TABLE, t, tumhash);
   #endif
   #ifdef HAVE_MYDB
      spaminess = mydbqry(mhash, t, cfg.rob_s, cfg.rob_x, tumhash);
   #endif
   }


   /* whether to include unknown tokens, 2006.02.15, SJ */

   if(spaminess < DEFAULT_SPAMICITY - cfg.exclusion_radius || spaminess > DEFAULT_SPAMICITY + cfg.exclusion_radius){

      if(strchr(p, '+') || strchr(p, '*'))
         n_phrases += addnode(s_phrase_hash, p, spaminess, DEVIATION(spaminess));

      if(strchr(p, '+') == NULL)
         n_tokens += addnode(shash, p, spaminess, DEVIATION(spaminess));

   }

   return 0;
}


#ifdef HAVE_MYSQL
int walk_hash(MYSQL mysql, struct node *xhash[MAXHASH], struct __config cfg){
#endif
#ifdef HAVE_SQLITE3
int walk_hash(sqlite3 *db, struct node *xhash[MAXHASH], struct __config cfg){
#endif
#ifdef HAVE_MYDB
int walk_hash(struct node *xhash[MAXHASH], struct __config cfg){
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
         assign_spaminess(p->str, cfg, QRY.uid);
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

struct _state parse_message(char *spamfile, struct __config cfg){
   FILE *f;
   char buf[MAXBUFSIZE];
   struct _state state;

   state = init_state();

   f = fopen(spamfile, "r");
   if(!f){
      syslog(LOG_PRIORITY, "%s: cannot open", spamfile);
      return state;
   }

   while(fgets(buf, MAXBUFSIZE-1, f))
      state = parse(buf, state);

   fclose(f);

   return state;
}


/*
 * TUM training if we have to
 */

#ifdef HAVE_MYSQL
int tum_train(MYSQL mysql, char *spamfile, double spaminess, struct __config cfg){
#endif
#ifdef HAVE_SQLITE3
int tum_train(sqlite3 *db, char *spamfile, double spaminess, struct __config cfg){
#endif
#ifdef HAVE_MYDB
int tum_train(char *spamfile, double spaminess, struct __config cfg){
#endif

   unsigned long n;
#ifndef HAVE_MYDB
   char buf[MAXBUFSIZE];
#endif

   if
#ifndef HAVE_MYDB
   ( (QRY.uid > 0 || cfg.group_type == GROUP_SHARED) && 
#endif

       (
         (cfg.training_mode == T_TUM && (spaminess >= cfg.spam_overall_limit || spaminess < cfg.max_ham_spamicity)) ||
         (cfg.initial_1000_learning == 1 && (QRY.ham_msg < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED || QRY.spam_msg < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED))
       )
#ifndef HAVE_MYDB
   )
#endif
   {

      gettimeofday(&tv1, &tz);

      if(spaminess >= cfg.spam_overall_limit){
      #ifdef HAVE_MYDB
         n = my_walk_hash(cfg.mydbfile, mhash, 1, tumhash, T_TOE);
      #else
         n = my_walk_hash(QRY, 1, SQL_TOKEN_TABLE, tumhash, T_TOE);
         snprintf(buf, MAXBUFSIZE-1, "update %s set nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
      #endif
      }
      else {
      #ifdef HAVE_MYDB
         n = my_walk_hash(cfg.mydbfile, mhash, 0, tumhash, T_TOE);
      #else
         n = my_walk_hash(QRY, 0, SQL_TOKEN_TABLE, tumhash, T_TOE);
         snprintf(buf, MAXBUFSIZE-1, "update %s set nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
      #endif
      }

      gettimeofday(&tv2, &tz);

      if(n > 0 && cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: TUM training %ld tokens for uid: %ld %ld [ms]", spamfile, n, QRY.uid, tvdiff(tv2, tv1)/1000);

   #ifdef HAVE_MYSQL
      mysql_real_query(&mysql, buf, strlen(buf));
   #endif
   #ifdef HAVE_SQLITE3
      if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK)
         sqlite3_step(pStmt);

      sqlite3_finalize(pStmt);
   #endif

      snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg.clapf_header_field);
   }

   clearhash(tumhash);


#ifdef HAVE_QCACHE
   close(QRY.sockfd);
#endif

   return 1;
}


/*
 * evaulate tokens
 */

#ifdef HAVE_MYSQL
double eval_tokens(MYSQL mysql, char *spamfile, struct __config cfg, struct _state state){
#endif
#ifdef HAVE_SQLITE3
double eval_tokens(sqlite3 *db, char *spamfile, struct __config cfg, struct _state state){
#endif
#ifdef HAVE_MYDB
double eval_tokens(char *spamfile, struct __config cfg, struct _state state){
#endif

   unsigned long n = 0;
   struct _token *p, *q;
   float spaminess, spaminess2;
   int i;
   struct node *Q;
#ifdef HAVE_SURBL
   struct node *urlhash[MAXHASH];
   struct node *P;
   char surbl_token[MAX_TOKEN_LEN];
   float n_urls = 0;
   int u, j, found_on_rbl=0;
#endif

   if(!state.first)
      return DEFAULT_SPAMICITY;

   surbl_match = 0;
   spaminess = spaminess2 = DEFAULT_SPAMICITY;

   p = state.first;

   inithash(shash);
   inithash(s_phrase_hash);

   inithash(B_hash);
   inithash(tumhash);

#ifdef HAVE_SURBL
   inithash(urlhash);
#endif

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
         assign_spaminess(p->str, cfg, QRY.uid);
      #endif

         addnode(B_hash, p->str, 0, 0);

      #ifdef HAVE_SURBL
         if(strncmp(p->str, "URL*", 4) == 0)
            n_urls += addnode(urlhash, p->str, 1, 1);
      #endif

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
         assign_spaminess(p->str, cfg, QRY.uid);
      #endif
      }

      else addnode(B_hash, p->str, 0, 0);


      if(n > 0){

         /* we may penalize embedded images, 2007.01.03, SJ */

         if(cfg.penalize_embed_images == 1 && strcmp(p->str, "src+cid") == 0){
            spaminess = REAL_SPAM_TOKEN_PROBABILITY;
            n_phrases += addnode(s_phrase_hash, "EMBED*", spaminess, DEVIATION(spaminess));
            n_tokens += addnode(shash, "EMBED*", spaminess, DEVIATION(spaminess));
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
   }

   /* add penalty for images, 2007.07.02, SJ */

   if(cfg.penalize_images == 1 && attachment_by_type(state, "image/") == 1){
       spaminess = REAL_SPAM_TOKEN_PROBABILITY;
       n_phrases += addnode(s_phrase_hash, "IMAGE*", spaminess, DEVIATION(spaminess));
       n_tokens += addnode(shash, "IMAGE*", spaminess, DEVIATION(spaminess));
   }


   if(state.n_subject_token == 0){
      spaminess = REAL_SPAM_TOKEN_PROBABILITY;
      n_phrases += addnode(s_phrase_hash, "NO_SUBJECT*", spaminess, DEVIATION(spaminess));
      n_tokens += addnode(shash, "NO_SUBJECT*", spaminess, DEVIATION(spaminess));
   }

   /* add the From line, 2007.06.16, SJ */

#ifdef HAVE_MYSQL
   assign_spaminess(mysql, state.from, cfg, QRY.uid);
#endif
#ifdef HAVE_SQLITE3
   assign_spaminess(db, state.from, cfg, QRY.uid);
#endif
#ifdef HAVE_MYDB
   assign_spaminess(state.from, cfg, QRY.uid);
#endif

   addnode(B_hash, state.from, 0, 0);

   if(state.unknown_client == 1){
      spaminess = REAL_SPAM_TOKEN_PROBABILITY;
      n_phrases += addnode(s_phrase_hash, "UNKNOWN_CLIENT*", spaminess, DEVIATION(spaminess));
      n_tokens += addnode(shash, "UNKNOWN_CLIENT*", spaminess, DEVIATION(spaminess));
   }

   /* redesigned spaminess calculation, 2007.08.28, SJ */

   if(cfg.use_pairs == 1){
      spaminess = sorthash(s_phrase_hash, MAX_PHRASES_TO_CHOOSE, cfg);
      if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_ham_spamicity && most_interesting_tokens(s_phrase_hash) < MAX_PHRASES_TO_CHOOSE)
         goto NEED_SINGLE_TOKENS;

      /* if we have no subject token to evaluate */

      if(state.n_subject_token == 0 && state.n_body_token < 10){
         goto NEED_SINGLE_TOKENS;
      }

   }
   else {
      NEED_SINGLE_TOKENS:
   #ifdef HAVE_MYSQL
      n_tokens += walk_hash(mysql, B_hash, cfg);
   #endif
   #ifdef HAVE_SQLITE3
      n_tokens += walk_hash(db, B_hash, cfg);
   #endif
   #ifdef HAVE_MYDB
      n_tokens += walk_hash(B_hash, cfg);
   #endif

      /* consult blacklists about the IPv4 address connecting to us */

   #ifdef HAVE_SURBL
      if(strlen(cfg.rbl_domain) > 3 && reverse_ipv4_addr(state.ip) == 1){

         gettimeofday(&tv1, &tz);
         found_on_rbl = rbl_list_check(cfg.rbl_domain, state.ip);
         gettimeofday(&tv2, &tz);
      #ifdef DEBUG
         fprintf(stderr, "rbl check took %ld ms\n", tvdiff(tv2, tv1)/1000);
      #else
         if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: rbl check took %ld ms", spamfile, tvdiff(tv2, tv1)/1000);
      #endif

         for(i=0; i<found_on_rbl; i++){
            snprintf(surbl_token, MAX_TOKEN_LEN-1, "RBL%d*%s", i, state.ip);
            n_phrases += addnode(s_phrase_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            n_tokens += addnode(shash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
         }
      }
   #endif


      /* add single tokens to token pairs, then recalculate spamicity */

      if(cfg.use_pairs == 1){
         for(i=0;i<MAXHASH;i++){
            Q = shash[i];
            while(Q != NULL){
               addnode(s_phrase_hash, Q->str, Q->spaminess, Q->deviation);
               Q = Q->r;
            }
         }
         spaminess = sorthash(s_phrase_hash, MAX_TOKENS_TO_CHOOSE, cfg);
      }

      if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_ham_spamicity && most_interesting_tokens(s_phrase_hash) < MAX_PHRASES_TO_CHOOSE)
         spaminess2 = sorthash(shash, MAX_TOKENS_TO_CHOOSE, cfg);

   }


#ifdef DEBUG
   fprintf(stderr, "phrase: %.4f, single token: %.4f\n", spaminess, spaminess2);
#endif

   if(DEVIATION(spaminess) < DEVIATION(spaminess2))
      spaminess = spaminess2;

   #ifdef DEBUG
      fprintf(stderr, "Bayesian result: %.4f\n", spaminess);
      fprintf(stderr, "%ld %ld %ld\n", state.c_hex_shit, state.c_shit, state.l_shit);
   #endif


   /* junk detection before the SURBL test, 2006.11.09, SJ */

   if(spaminess > cfg.max_ham_spamicity){

      if(state.base64_text == 1 && cfg.spaminess_of_text_and_base64 > 0){
         return cfg.spaminess_of_text_and_base64;
      }

      if(cfg.invalid_junk_limit > 0 && state.c_shit > cfg.invalid_junk_limit && spaminess < cfg.spam_overall_limit){
      #ifdef DEBUG
         fprintf(stderr, "invalid junk characters: %ld (limit: %d)\n", state.c_shit, cfg.invalid_junk_limit);
      #endif

         return cfg.spaminess_of_strange_language_stuff;
      }

      if(cfg.invalid_junk_line > 0 && state.l_shit >= cfg.invalid_junk_line && spaminess < cfg.spam_overall_limit){
      #ifdef DEBUG
         fprintf(stderr, "invalid junk lines: %ld (limit: %d)\n", state.l_shit, cfg.invalid_junk_line);
      #endif

         return cfg.spaminess_of_strange_language_stuff;
      }

      if(cfg.invalid_hex_junk_limit > 0 && state.c_hex_shit > cfg.invalid_hex_junk_limit && spaminess < cfg.spam_overall_limit){
      #ifdef DEBUG
         fprintf(stderr, "invalid hex. junk characters: %ld (limit: %d)\n", state.c_hex_shit, cfg.invalid_hex_junk_limit);
      #endif

         return cfg.spaminess_of_strange_language_stuff;
      }
   }

#ifdef HAVE_SURBL

   /*
     check URLs against the SUBRL database and recalculate spaminess if we are not sure
     that this message is certainly spam or ham, 2006.06.23, SJ
    */

   if(n_urls > 0 && spaminess > cfg.max_ham_spamicity && spaminess < cfg.spam_overall_limit && strlen(cfg.surbl_domain) > 2){
      spaminess = spaminess2 = 0.5;

      for(u=0; u < MAXHASH; u++){
         Q = urlhash[u];
         while(Q != NULL){
            P = Q;

            if(strncmp(P->str, "URL*", 4) == 0 && strchr(P->str, '+') == NULL){
               gettimeofday(&tv1, &tz);
               i = rbl_list_check(cfg.surbl_domain, P->str+4);
               gettimeofday(&tv2, &tz);
            #ifdef DEBUG
               fprintf(stderr, "surbl check took %ld ms\n", tvdiff(tv2, tv1)/1000);
            #else
               if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: surbl check took %ld ms", spamfile, tvdiff(tv2, tv1)/1000);
            #endif

               surbl_match += i;

               for(j=0; j<i; j++){
                  snprintf(surbl_token, MAX_TOKEN_LEN-1, "SURBL%d*%s", j, P->str+4);
                  n_phrases += addnode(s_phrase_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
                  n_tokens += addnode(shash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
               }
            }

            Q = Q->r;
            if(P)
               free(P);
         }
         urlhash[u] = NULL;
      }

      if(cfg.use_pairs == 1) spaminess = sorthash(s_phrase_hash, MAX_PHRASES_TO_CHOOSE, cfg);

      if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_ham_spamicity && most_interesting_tokens(s_phrase_hash) < MAX_PHRASES_TO_CHOOSE){
      //if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_ham_spamicity){
         if(n_tokens < 8){
         #ifdef HAVE_MYSQL
            n_tokens += walk_hash(mysql, B_hash, cfg);
         #endif
         #ifdef HAVE_SQLITE3
            n_tokens += walk_hash(db, B_hash, cfg);
         #endif
         #ifdef HAVE_MYDB
            n_tokens += walk_hash(B_hash, cfg);
         #endif
         }

         spaminess2 = sorthash(shash, MAX_TOKENS_TO_CHOOSE, cfg);
      }


      if(DEVIATION(spaminess) < DEVIATION(spaminess2))
         spaminess = spaminess2;

   #ifdef DEBUG
      fprintf(stderr, "phrase: %.4f, single token: %.4f\n", spaminess, spaminess2);
      fprintf(stderr, "Bayesian result after surbl test: %f\n", spaminess);
      fprintf(stderr, "surbl matches: %d\n", surbl_match);
   #endif

   }

#endif


#ifdef HAVE_MYDB
 #ifndef DEBUG
   u = update_tokens(cfg.mydbfile, mhash, s_phrase_hash);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "update timestamp for %d phrases", u);
   u = update_tokens(cfg.mydbfile, mhash, shash);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "update timestamp for %d single tokens", u);
 #endif
#endif

   clearhash(shash);
   clearhash(s_phrase_hash);
   clearhash(B_hash);


   /* TUM training was here ... */


#ifdef HAVE_SURBL
   if(spaminess > cfg.max_ham_spamicity && spaminess < cfg.spam_overall_limit && cfg.rude_surbl > 0 && surbl_match >= cfg.rude_surbl)
      return cfg.spaminess_of_caught_by_surbl;

   /* if the message is not good enough and found on a blacklist, mark it as spam, 2007.09.11, SJ */

   if(spaminess > cfg.max_ham_spamicity && spaminess < cfg.spam_overall_limit && found_on_rbl > 0)
      return cfg.spaminess_of_caught_by_surbl;
#endif


   /* if we shall mark the message as spam because of the embedded image */

   if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_ham_spamicity && has_embed_image == 1){
      return cfg.spaminess_of_embed_image;
   }


   /* fix spaminess value if we have to */

   if(spaminess < 0) spaminess = REAL_HAM_TOKEN_PROBABILITY;
   if(spaminess > 1) spaminess = REAL_SPAM_TOKEN_PROBABILITY;

   return spaminess;
}


/*
 * Bayesian result of the file
 */

#ifdef HAVE_MYSQL
double bayes_file(MYSQL mysql, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg){
#endif
#ifdef HAVE_SQLITE3
double bayes_file(sqlite3 *db, char *spamfile, struct _state state, struct session_data sdata, struct __config cfg){
#endif
#ifdef HAVE_MYDB
double bayes_file(char *spamfile, struct _state state, struct session_data sdata, struct __config cfg){
#endif

   char buf[MAXBUFSIZE], *p;
   float spaminess, ham_from=0, spam_from=0;

#ifdef HAVE_MYSQL
   struct te TE;
#endif
#ifdef HAVE_MYDB
   struct mydb_node *Q;
#endif

   if(spamfile == NULL){
      syslog(LOG_PRIORITY, "%s: no spamfile", sdata.ttmpfile);
      return DEFAULT_SPAMICITY;
   }


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


   p = strrchr(spamfile, '/');
   if(p)
      p++;
   else
      p = spamfile;


   /* evaluate the blackhole result, 2006.10.02, SJ */

#ifdef HAVE_BLACKHOLE
   if(strlen(cfg.blackhole_path) > 3 && blackness(cfg.blackhole_path, state.ip, cfg.verbosity) > 100){
      syslog(LOG_PRIORITY, "%s: found %s on our blackhole", p, state.ip);

      return cfg.spaminess_of_blackholed_mail;
   }
#endif

   /* switch to shared group if we are using mydb, 2007.10.09, SJ */

#ifdef HAVE_MYDB
   cfg.group_type = GROUP_SHARED;
#endif

   /* fix uid and sql statement if this is a shared group */

   if(cfg.group_type == GROUP_SHARED){
      QRY.uid = 0;
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);
   }

   /* fix sql statment if we use a merged group */

   if(cfg.group_type == GROUP_MERGED)
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0 OR uid=%ld", SQL_MISC_TABLE, QRY.uid);

#ifdef DEBUG
   fprintf(stderr, "uid: %ld\n", QRY.uid);
#endif

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
   QRY.ham_msg = Nham;
   QRY.spam_msg = Nspam;
#endif

   if((QRY.ham_msg + QRY.spam_msg == 0) && cfg.initial_1000_learning == 0){
      syslog(LOG_PRIORITY, "%s: %s", p, ERR_MYSQL_DATA);
      return DEFAULT_SPAMICITY;
   }

#ifdef DEBUG
   fprintf(stderr, "nham: %.0f, nspam: %.0f\n", QRY.ham_msg, QRY.spam_msg);
#endif

   /*
    * auto whitelist test, 2007.06.21, SJ
    * it may be better to only count the users own experiment with this sender...
    * A further note: this way we will not TUM train the message if it's whitelisted
    */

   if(cfg.enable_auto_white_list == 1){

   #ifdef HAVE_MYSQL
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND  (uid=0 OR uid=%ld)", SQL_TOKEN_TABLE, APHash(state.from), QRY.uid);
      TE = get_ham_spam(mysql, buf);
      ham_from = TE.nham;
      spam_from = TE.nspam;
   #endif
   #ifdef HAVE_SQLITE3
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token='%llu' AND  (uid=0 OR uid=%ld)", SQL_TOKEN_TABLE, APHash(state.from), QRY.uid);
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

   #ifdef DEBUG
      fprintf(stderr, "from: %.0f, %.0f\n", ham_from, spam_from);
   #endif

      if(ham_from > NUMBER_OF_GOOD_FROM && spam_from == 0){
         return REAL_HAM_TOKEN_PROBABILITY;
      }
   }


/* Query cache support */

#ifdef HAVE_QCACHE
   QRY.sockfd = qcache_socket(cfg.qcache_addr, cfg.qcache_port, cfg.qcache_socket);
   if(QRY.sockfd == -1)
      return DEFAULT_SPAMICITY;
#endif


   /* evaluate the tokens */

#ifdef HAVE_MYSQL
   spaminess = eval_tokens(mysql, p, cfg, state);
#endif
#ifdef HAVE_SQLITE3
   spaminess = eval_tokens(db, p, cfg, state);
#endif
#ifdef HAVE_MYDB
   spaminess = eval_tokens(p, cfg, state);
#endif


   return spaminess;
}


/*
 * retrain the message
 */

#ifdef HAVE_MYSQL
int retraining(MYSQL mysql, struct session_data sdata, char *username, int is_spam, struct __config cfg){
#endif
#ifdef HAVE_SQLITE3
int retraining(sqlite3 *db, struct session_data sdata, char *username, int is_spam, struct __config cfg){
#endif
#ifdef HAVE_MYDB
int retraining(struct session_data sdata, char *username, int is_spam, struct __config cfg){
#endif

   int fd, len, i=0, m, train_mode=T_TOE;
   char *p, *q, *r, ID[RND_STR_LEN+1]="", buf[8*MAXBUFSIZE], puf[SMALLBUFSIZE];
   unsigned long now;
   double spaminess;
   struct _state state;
   struct _token *P, *Q;
   struct node *tokens[MAXHASH];
   time_t cclock;


   if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: trying to retrain: num of rcpt: %d, uid: %ld, username: %s", sdata.ttmpfile, sdata.num_of_rcpt_to, sdata.uid, username);

   /* have we got valid data? */

   if(sdata.num_of_rcpt_to != 1) return 1;
   if(sdata.uid == 0) return 1;
   if(username == NULL) return 1;

   /* make sure nothing interferes with us */

   cfg.training_mode = 0;
   cfg.initial_1000_learning=0;

   cfg.rbl_domain[0] = '\0';
   cfg.surbl_domain[0] = '\0';

   cfg.blackhole_path[0] = '\0';

   time(&cclock);
   now = cclock;


   /* if we want to train with a message having no message id in it,
      such as a blackhole message, 2007.10.18, SJ
    */

   if(sdata.skip_id_check == 1){
      snprintf(ID, RND_STR_LEN, "%s", sdata.ttmpfile);
      goto AFTER_ID_EXTRACT;
   }

   fd = open(sdata.ttmpfile, O_RDONLY);
   if(fd != -1){
      while((len = read(fd, buf, 8*MAXBUFSIZE)) > 0){
         /* data should be here in the first read */

         p = buf;
         do {
            p = split(p, '\n', puf, SMALLBUFSIZE-1);

            q = strstr(puf, cfg.clapf_header_field);
            if(q){
               trim(puf);
               r = strchr(puf, ' ');
               if(r){
                  r++;
                  if(is_valid_id(r)){
                     i++;
                     if(i <= 1){
                        snprintf(ID, RND_STR_LEN, "%s", r);
                     }
                  }
               }
            }

            if(strlen(ID) > 2 && strncmp(puf, cfg.clapf_header_field, strlen(cfg.clapf_header_field)) == 0){
               if(strncmp(puf + strlen(cfg.clapf_header_field), "TUM", 3) == 0)
                  train_mode = T_TUM;
            }
         } while(p);
            
      }

      close(fd);
   }
   else return 1;


AFTER_ID_EXTRACT:

   if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: found id: %s, is_spam: %d", sdata.ttmpfile, ID, is_spam);

   /* determine the path of the original file */

   if(is_spam == 1)
      snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s/h.%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username, ID);
   else
      snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s/s.%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username, ID);

   /* fix the queue file if we haven't renamed the file according to its spamicity status, 2007.10.24, SJ */
   if(sdata.skip_id_check == 1)
      snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s/%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username, ID);

   /* if we use the pop3 proxy on the localhost, 2007.11.21, SJ */
#ifdef HAVE_POP3GW
   snprintf(buf, MAXBUFSIZE-1, "%s", ID);
#endif

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

   state = parse_message(buf, cfg);


   /* if this is a shared group, make sure the token database is trained with uid=0 */

   if(cfg.group_type == GROUP_SHARED)
      QRY.uid = 0;

   if(state.first){
     for(i=0; i<MAX_ITERATIVE_TRAIN_LOOPS; i++){

         inithash(tokens);

         spaminess = DEFAULT_SPAMICITY;
         P = state.first;

         while(P != NULL){
            Q = P->r;
            m = 1;

            addnode(tokens, P->str, 0, 0);
            P = Q;
         }

         QRY.sockfd = -1;

      #ifdef HAVE_QCACHE
         QRY.sockfd = qcache_socket(cfg.qcache_addr, cfg.qcache_port, cfg.qcache_socket);
      #endif

      #ifdef HAVE_MYDB
         my_walk_hash(cfg.mydbfile, mhash, is_spam, tokens, train_mode);
      #else
         my_walk_hash(QRY, is_spam, SQL_TOKEN_TABLE, tokens, train_mode);
      #endif

         if(QRY.sockfd != -1) close(QRY.sockfd);

         if(i == 0){

            if(is_spam == 1){
               if(train_mode == T_TUM)
                  snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1, nham=nham-1 WHERE uid=%ld AND nham > 0", SQL_MISC_TABLE, QRY.uid);
               else
                  snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
            }
            else {
               if(train_mode == T_TUM)
                  snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1, nspam=nspam-1 WHERE uid=%ld AND nspam > 0", SQL_MISC_TABLE, QRY.uid);
               else
                  snprintf(buf, MAXBUFSIZE-1, "UPDATE %s SET nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
            }

         #ifdef HAVE_MYSQL   
            mysql_real_query(&mysql, buf, strlen(buf));
         #endif
         #ifdef HAVE_SQLITE3
            sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail);
            sqlite3_step(pStmt);
            sqlite3_finalize(pStmt);
         #endif

            snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (uid, ts, msgid, is_spam) VALUES(%ld, %ld, '%s', %d)", SQL_TRAININGLOG_TABLE, QRY.uid, now, ID, is_spam);

         #ifdef HAVE_MYSQL
            mysql_real_query(&mysql, buf, strlen(buf));
         #endif
         #ifdef HAVE_SQLITE3
            sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail);
            sqlite3_step(pStmt);
            sqlite3_finalize(pStmt);
         #endif


            if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: training, mode: %d", ID, train_mode);
         }

         clearhash(tokens);

      #ifdef HAVE_MYSQL
         spaminess = bayes_file(mysql, sdata.ttmpfile, state, sdata, cfg);
      #endif
      #ifdef HAVE_SQLITE3
         spaminess = bayes_file(db, sdata.ttmpfile, state, sdata, cfg);
      #endif
      #ifdef HAVE_MYDB
         spaminess = bayes_file(sdata.ttmpfile, state, sdata, cfg);
      #endif

      #ifdef HAVE_QCACHE
         close(QRY.sockfd);
      #endif

         if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: training round %d, spaminess: %.4f", ID, i, spaminess);

         if(is_spam == 1 && spaminess > cfg.spam_overall_limit) break;
         if(is_spam == 0 && spaminess < cfg.max_ham_spamicity) break;

         /* only the first round can be TUM/TEFT, 2007.09.14, SJ */
         train_mode = T_TOE;
      }

   }

   free_and_print_list(state.first, 0);

   return 0;
}

