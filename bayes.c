/*
 * bayes.c, 2007.08.01, SJ
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
#endif

int my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode);

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
#endif

#ifdef HAVE_CDB
   #include <cdb.h>
   #include "cdb1.h"

   int init_cdbs(char *tokensfile);
   void close_cdbs();
   float cdbqry(struct cdb CDB, char *s);
#endif

float SQL_QUERY(qry QRY, char *tokentable, char *token, struct node *xhash[MAXHASH]);



/*
 * assign spaminess value to token
 */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
int assign_spaminess(MYSQL mysql, char *p, struct __config cfg, unsigned int uid){
#endif
#ifdef HAVE_SQLITE3
int assign_spaminess(sqlite3 *db, char *p, struct __config cfg, unsigned int uid){
#endif
#ifdef HAVE_CDB
int assign_spaminess(char *p, struct __config cfg, unsigned int uid){
#endif

   float spaminess=0;
   char t[MAX_TOKEN_LEN], *s;

   /* if we already have this token, 2006.03.13, SJ */

   if(findnode(shash, p))
      return 0;

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   spaminess = SQL_QUERY(QRY, SQL_TOKEN_TABLE, p, tumhash);
#endif
#ifdef HAVE_SQLITE3
   spaminess = SQL_QUERY(QRY, SQL_TOKEN_TABLE, p, tumhash);
#endif
#ifdef HAVE_CDB
   spaminess = cdbqry(tokenscdb, p);
#endif

   /* if it was at the Subject: header line, let's try it if it were not in the Subject line, 2006.05.03, SJ */

   if(spaminess < DEFAULT_SPAMICITY_HIGH && spaminess > DEFAULT_SPAMICITY_LOW && strncasecmp(p, "Subject*", 8) == 0){
      memset(t, 0, MAX_TOKEN_LEN);

      s = strchr(p, '+');
      if(s){
         *(s+1) = '\0';
         strncpy(t, p+8, MAX_TOKEN_LEN-1);
         strncat(t, s+1+8, MAX_TOKEN_LEN-1);
      }
      else
         strncpy(t, p+8, MAX_TOKEN_LEN-1);

   #ifdef HAVE_MYSQL_TOKEN_DATABASE
      spaminess = SQL_QUERY(QRY, SQL_TOKEN_TABLE, t, tumhash);
   #endif
   #ifdef HAVE_SQLITE3
      spaminess = SQL_QUERY(QRY, SQL_TOKEN_TABLE, t, tumhash);
   #endif
   #ifdef HAVE_CDB
      spaminess = cdbqry(tokenscdb, t);
   #endif

   }


   /* whether to include unknown tokens, 2006.02.15, SJ */

   if(spaminess < DEFAULT_SPAMICITY - cfg.exclusion_radius || spaminess > DEFAULT_SPAMICITY + cfg.exclusion_radius){
      /*
       * add token pairs and URLs and other special tokens to s_phrase_hash, 2006.03.13, SJ
       */

      if(strchr(p, '+') || strncasecmp(p, "URL*", 4) == 0)
         n_phrases += addnode(s_phrase_hash, p, spaminess, DEVIATION(spaminess));

      if(strchr(p, '+') == NULL)
         n_tokens += addnode(shash, p, spaminess, DEVIATION(spaminess));
   }

   return 0;
}


#ifdef HAVE_MYSQL_TOKEN_DATABASE
int walk_hash(MYSQL mysql, struct node *xhash[MAXHASH], struct __config cfg){
#endif
#ifdef HAVE_SQLITE3
int walk_hash(sqlite3 *db, struct node *xhash[MAXHASH], struct __config cfg){
#endif
#ifdef HAVE_CDB
int walk_hash(struct node *xhash[MAXHASH], struct __config cfg){
#endif

   int i, n=0;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

      #ifdef HAVE_MYSQL_TOKEN_DATABASE
         assign_spaminess(mysql, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_SQLITE3
         assign_spaminess(db, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_CDB
         assign_spaminess(p->str, cfg, QRY.uid);
      #endif

         n++;

         q = q->r;
      }
   }

   return n;
}

/*
 * evaulate tokens
 */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
double eval_tokens(MYSQL mysql, char *spamfile, struct __config cfg, struct _state state){
#endif
#ifdef HAVE_SQLITE3
double eval_tokens(sqlite3 *db, char *spamfile, struct __config cfg, struct _state state){
#endif
#ifdef HAVE_CDB
double eval_tokens(char *spamfile, struct __config cfg, struct _state state){
#endif

   unsigned long n=0;
   struct _token *p, *q;
   float spaminess, spaminess2;
   int i;
   char buf[MAXBUFSIZE];
   struct node *Q;
#ifdef HAVE_SURBL
   struct node *urlhash[MAXHASH];
   struct node *P;
   char *qq, surbl_token[MAX_TOKEN_LEN], rbldomain[MAX_TOKEN_LEN];
   float n_urls = 0;
   int u;
#endif

   if(!state.first)
      return DEFAULT_SPAMICITY;

   surbl_match = 0;
   spaminess = spaminess2 = 0.5;

   p = state.first;
   n = 0;

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
      #ifdef HAVE_MYSQL_TOKEN_DATABASE
         assign_spaminess(mysql, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_SQLITE3
         assign_spaminess(db, p->str, cfg, QRY.uid);
      #endif

         addnode(B_hash, p->str, 0, 0);

      #ifdef HAVE_SURBL
         if(strncmp(p->str, "URL*", 4) == 0)
            n_urls += addnode(urlhash, p->str, 1, 1);
      #endif

      }

      /* 2007.06.06, SJ */

      if(cfg.use_pairs == 1 && strchr(p->str, '+')){
      #ifdef HAVE_MYSQL_TOKEN_DATABASE
         assign_spaminess(mysql, p->str, cfg, QRY.uid);
      #endif
      #ifdef HAVE_SQLITE3
         assign_spaminess(db, p->str, cfg, QRY.uid);
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
       || attachment_by_type(state, "application/vnd.ms-excel") == 1)){
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


   /* add the From line, 2007.06.16, SJ */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   assign_spaminess(mysql, state.from, cfg, QRY.uid);
#endif
#ifdef HAVE_SQLITE3
   assign_spaminess(db, state.from, cfg, QRY.uid);
#endif

   addnode(B_hash, state.from, 0, 0);


   /* redesigned spaminess calculation, 2007.05.21, SJ */

   spaminess = sorthash(s_phrase_hash, MAX_PHRASES_TO_CHOOSE, cfg);

   if(n_phrases > cfg.min_phrase_number){

      /*
       * if we are unsure and we have not enough truly interesting tokens, add the single tokens, too, 2007.07.15, SJ
       */

      if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_junk_spamicity && most_interesting_tokens(s_phrase_hash) < MAX_PHRASES_TO_CHOOSE){
      //if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_junk_spamicity){
         goto NEED_SINGLE_TOKENS;
      }
   }
   else {
      NEED_SINGLE_TOKENS:

   #ifdef HAVE_MYSQL_TOKEN_DATABASE
      n_tokens += walk_hash(mysql, B_hash, cfg);
   #endif
   #ifdef HAVE_SQLITE3
      n_tokens += walk_hash(db, B_hash, cfg);
   #endif

      for(i=0;i<MAXHASH;i++){
         Q = shash[i];
         while(Q != NULL){
            addnode(s_phrase_hash, Q->str, Q->spaminess, Q->deviation);
            Q = Q->r;
         }
      }

      spaminess = sorthash(s_phrase_hash, MAX_TOKENS_TO_CHOOSE, cfg);
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

   if(spaminess > cfg.max_junk_spamicity){

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

   if(n_urls > 0 && spaminess > cfg.max_junk_spamicity && spaminess < cfg.spam_overall_limit){
      spaminess = spaminess2 = 0.5;

      for(u=0; u < MAXHASH; u++){
         Q = urlhash[u];
         while(Q != NULL){
            P = Q;

            if(strlen(cfg.surbl_domain) > 2 && strncmp(P->str, "URL*", 4) == 0 && strchr(P->str, '+') == NULL){
               i = 0;
               qq = cfg.surbl_domain;
               do {
                  qq = split(qq, ',', rbldomain, MAX_TOKEN_LEN-1);
                  snprintf(surbl_token, MAX_TOKEN_LEN-1, "%s%d*%s", "SURBL", i, P->str+4);

                  if(rbl_check(rbldomain, P->str+4) == 1){
                     spaminess = REAL_SPAM_TOKEN_PROBABILITY;
                     surbl_match++;
                     n_phrases += addnode(s_phrase_hash, surbl_token, spaminess, DEVIATION(spaminess));
                     n_tokens += addnode(shash, surbl_token, spaminess, DEVIATION(spaminess));
                  }

                  i++;
               } while(qq);
            }

            Q = Q->r;
            if(P)
               free(P);
         }
         urlhash[u] = NULL;
      }

      spaminess = sorthash(s_phrase_hash, MAX_PHRASES_TO_CHOOSE, cfg);

      if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_junk_spamicity && most_interesting_tokens(s_phrase_hash) < MAX_PHRASES_TO_CHOOSE){
      //if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_junk_spamicity){
         if(n_tokens < 8){
         #ifdef HAVE_MYSQL_TOKEN_DATABASE
            n_tokens += walk_hash(mysql, B_hash, cfg);
         #endif
         #ifdef HAVE_SQLITE3
            n_tokens += walk_hash(db, B_hash, cfg);
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


   clearhash(shash);
   clearhash(s_phrase_hash);
   clearhash(B_hash);


   /*
    * train with tokens not mature enough, if we are using TUM mode
    * AND
    * we are certain about the message (ie. spam or ham)
    * AND
    * we are using a shared group or we found the user
    */

#ifndef HAVE_CDB
   if(cfg.training_mode == T_TUM && (QRY.uid > 0 || cfg.group_type == GROUP_SHARED) && (spaminess >= cfg.spam_overall_limit || spaminess < cfg.max_junk_spamicity)){
      gettimeofday(&tv1, &tz);

      if(spaminess >= cfg.spam_overall_limit){
         n = my_walk_hash(QRY, 1, SQL_TOKEN_TABLE, tumhash, T_TOE);
         snprintf(buf, MAXBUFSIZE-1, "update %s set nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
      }
      else {
         n = my_walk_hash(QRY, 0, SQL_TOKEN_TABLE, tumhash, T_TOE);
         snprintf(buf, MAXBUFSIZE-1, "update %s set nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, QRY.uid);
      }

      gettimeofday(&tv2, &tz);

      syslog(LOG_PRIORITY, "%s: TUM training %ld tokens for uid: %ld %ld [ms]", spamfile, n, QRY.uid, tvdiff(tv2, tv1)/1000);

   #ifdef HAVE_MYSQL_TOKEN_DATABASE
      mysql_real_query(&mysql, buf, strlen(buf));
   #endif
   #ifdef HAVE_SQLITE3
      if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK)
         sqlite3_step(pStmt);

      sqlite3_finalize(pStmt);
   #endif

      snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg.clapf_header_field);
   }
#endif

   clearhash(tumhash);


#ifdef HAVE_SURBL
   if(spaminess > cfg.max_junk_spamicity && spaminess < cfg.spam_overall_limit && cfg.rude_surbl > 0 && surbl_match >= cfg.rude_surbl)
      return cfg.spaminess_of_caught_by_surbl;
#endif


   /* fix spaminess value if we have to */

   if(spaminess < 0) spaminess = REAL_HAM_TOKEN_PROBABILITY;
   if(spaminess > 1) spaminess = REAL_SPAM_TOKEN_PROBABILITY;

   return spaminess;
}


/*
 * Bayesian result of the file
 */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
double bayes_file(MYSQL mysql, char *spamfile, struct session_data sdata, struct __config cfg){
#endif
#ifdef HAVE_SQLITE3
double bayes_file(sqlite3 *db, char *spamfile, struct session_data sdata, struct __config cfg){
#endif
#ifdef HAVE_CDB
double bayes_file(char *cdbfile, char *spamfile, struct session_data sdata, struct __config cfg){
#endif

   struct _state state;
   char buf[MAXBUFSIZE], *p, *q;
   float spaminess, ham_from=0, spam_from=0;
   FILE *F;

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   struct te TE;
#endif

   if(spamfile == NULL)
      return ERR_BAYES_NO_SPAM_FILE;

   state = init_state();

   /* init query structure */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   QRY.mysql = mysql;
#endif
#ifdef HAVE_SQLITE3
   QRY.db = db;
#endif
   //QRY.uid = 0;
   QRY.uid = sdata.uid;
   QRY.sockfd = -1;
   QRY.ham_msg = 0;
   QRY.spam_msg = 0;
   QRY.rob_s = cfg.rob_s;
   QRY.rob_x = cfg.rob_x;

   F = fopen(spamfile, "r");
   if(!F)
      return ERR_BAYES_OPEN_SPAM_FILE;

   p = strrchr(spamfile, '/');
   if(p)
      p++;
   else
      p = spamfile;


   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: parsing", p);

   while(fgets(buf, MAXBUFSIZE-1, F))
      state = parse(buf, state);

   fclose(F);



   /* mark base64 encoded textual messages as spam, 2006.01.02, SJ */

   if(state.base64_text == 1 && cfg.spaminess_of_text_and_base64 > 0){
      free_and_print_list(state.first, 0);
      return cfg.spaminess_of_text_and_base64;
   }

 
   /* evaluate the blackhole result, 2006.10.02, SJ */

#ifdef HAVE_BLACKHOLE
   if(strlen(cfg.blackhole_path) > 3 && blackness(cfg.blackhole_path, state.ip, cfg.verbosity) > 100){
      /* free token list first */
      free_and_print_list(state.first, 0);
      syslog(LOG_PRIORITY, "%s: found %s on our blackhole", p, state.ip);

      return cfg.spaminess_of_blackholed_mail;
   }
#endif



   /*
    * determine uid if we use a merged group, 2007.06.13, SJ
    */

   if(cfg.group_type == GROUP_MERGED && QRY.uid == 0){

      if(cfg.training_mode == T_TUM && sdata.num_of_rcpt_to == 1){
         if(strlen(sdata.rcptto[0]) > 3){
            q = strchr(sdata.rcptto[0], '>');
            if(q){
               *q = '\0';
               q = strchr(sdata.rcptto[0], '<');
               if(q) snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", SQL_USER_TABLE, ++q);
            }
         }
         else if((q = getenv("FROM")))
            snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", SQL_USER_TABLE, q);

         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "stmt for uid: %s", buf);

      #ifdef HAVE_MYSQL_TOKEN_DATABASE 
         QRY.uid = get_uid(mysql, buf);
      #endif
      #ifdef HAVE_SQLITE3
         if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
            while(sqlite3_step(pStmt) == SQLITE_ROW){
               QRY.uid = sqlite3_column_int(pStmt, 0);
            }
         }
         sqlite3_finalize(pStmt);
      #endif

      }
      else if(sdata.num_of_rcpt_to == -1) QRY.uid = sdata.uid;

      snprintf(buf, MAXBUFSIZE-1, "SELECT SUM(nham), SUM(nspam) FROM %s WHERE uid=0 OR uid=%ld", SQL_MISC_TABLE, QRY.uid);
   }
   else {
      /* or this is a shared group */
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);
   }

#ifdef DEBUG
   fprintf(stderr, "uid: %ld\n", QRY.uid);
#endif

   /*
    * select the number of ham and spam messages, and return error if less than 1
    */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
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

#ifndef HAVE_CDB
   if(QRY.ham_msg <= 0 || QRY.spam_msg <= 0){
      free_and_print_list(state.first, 0);
      syslog(LOG_PRIORITY, "%s: %s", p, ERR_MYSQL_DATA);
      return ERR_BAYES_NO_TOKEN_FILE;
   }
#endif

#ifdef DEBUG
   fprintf(stderr, "nham: %.0f, nspam: %.0f\n", QRY.ham_msg, QRY.spam_msg);
#endif

   /*
    * auto whitelist test, 2007.06.21, SJ
    * it may be better to only count the users own experiment with this sender...
    * A further note: this way we will not TUM train the message if it's whitelisted
    */

   if(cfg.enable_auto_white_list == 1){

   #ifdef HAVE_MYSQL_TOKEN_DATABASE
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


   #ifdef DEBUG
      fprintf(stderr, "from: %.0f, %.0f\n", ham_from, spam_from);
   #endif

      if(ham_from > NUMBER_OF_GOOD_FROM && spam_from == 0){
         free_and_print_list(state.first, 0);
         return REAL_HAM_TOKEN_PROBABILITY;
      }
   }



#ifdef HAVE_CDB
   if(!cfg.tokensfile)
      return ERR_BAYES_NO_TOKEN_FILE;

   if(init_cdbs(cfg.tokensfile) == 0)
      return DEFAULT_SPAMICITY;
#endif


/* Query cache support */

#ifdef HAVE_QCACHE
   QRY.sockfd = qcache_socket(cfg.qcache_addr, cfg.qcache_port, cfg.qcache_socket);
   if(QRY.sockfd == -1)
      return DEFAULT_SPAMICITY;
#endif


   /* evaluate the tokens */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   spaminess = eval_tokens(mysql, p, cfg, state);
#endif
#ifdef HAVE_SQLITE3
   spaminess = eval_tokens(db, p, cfg, state);
#endif
#ifdef HAVE_CDB
   spaminess = eval_tokens(p, cfg, state);
#endif

   free_and_print_list(state.first, 0);

#ifdef HAVE_QCACHE
   close(QRY.sockfd);
#endif

   /* if we shall mark the message as spam because of the embedded image */

   if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_embed_image_spamicity && has_embed_image == 1){
      return cfg.spaminess_of_embed_image;
   }

#ifdef HAVE_CDB
   close_cdbs(tokenscdb);
#endif

   return spaminess;
}

