/*
 * bayes.c, 2007.07.06, SJ
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
#include "hdr.h"
#include "messages.h"
#include "config.h"

struct node *s_phrase_hash[MAXHASH], *shash[MAXHASH];
struct node *B_hash[MAXHASH], *tumhash[MAXHASH];
float n_phrases = 0;
float n_tokens = 0;

int surbl_match = 0;
int has_embed_image = 0;
int sd=-1;

unsigned long uid = 0;

struct timezone tz;
struct timeval tv1, tv2;


#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL_RES *res;
   MYSQL_ROW row;
   int mysql_conn = 0;
#endif

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   float myqry(MYSQL mysql, int sockfd, char *tokentable, char *token, float ham_msg, float spam_msg, unsigned int uid, struct node *xhash[MAXHASH]);
   int my_walk_hash(MYSQL mysql, int sockfd, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], unsigned int uid, int train_mode);

   float ham_msg=0, spam_msg=0;
#endif

#ifdef HAVE_CDB
   #include <cdb.h>
   #include "cdb1.h"

   int init_cdbs(char *tokensfile);
   void close_cdbs();
   float cdbqry(struct cdb CDB, char *s);
#endif

int is_header_field(char *s){
   int i;
   char *p;

   for(i=0; i<(sizeof(mail_headers) / sizeof(p)); i++){
      if(strncmp(s, mail_headers[i], strlen(mail_headers[i])) == 0)
         return 1;
   }
   return 0;
}


/*
 * assign spaminess value to token
 */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
int assign_spaminess(MYSQL mysql, char *p, struct __config cfg, unsigned int uid){
#else
int assign_spaminess(char *p, struct __config cfg, unsigned int uid){
#endif

   float spaminess=0;
   char t[MAX_TOKEN_LEN], *s;

   /* if we already have this token, 2006.03.13, SJ */

   if(findnode(shash, p))
      return 0;

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   spaminess = myqry(mysql, sd, cfg.mysqltokentable, p, ham_msg, spam_msg, uid, tumhash);
#else
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
      spaminess = myqry(mysql, sd, cfg.mysqltokentable, t, ham_msg, spam_msg, uid, tumhash);
   #else
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
#else
int walk_hash(struct node *xhash[MAXHASH], struct __config cfg){
#endif

   int i, n=0;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

      #ifdef HAVE_MYSQL_TOKEN_DATABASE
         assign_spaminess(mysql, p->str, cfg, uid);
      #else
         assign_spaminess(p->str, cfg, uid);
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
#else
double eval_tokens(char *spamfile, struct __config cfg, struct _state state){
#endif

   unsigned long n=0;
   struct _token *p, *q;
   float spaminess, spaminess2;
#ifdef HAVE_SURBL
   struct node *urlhash[MAXHASH];
   struct node *P, *Q;
   char *qq, surbl_token[MAX_TOKEN_LEN], rbldomain[MAX_TOKEN_LEN], buf[MAXBUFSIZE];
   float n_urls = 0;
   int u, i;
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
         assign_spaminess(mysql, p->str, cfg, uid);

         addnode(B_hash, p->str, 0, 0);

      #ifdef HAVE_SURBL
         if(strncmp(p->str, "URL*", 4) == 0)
            n_urls += addnode(urlhash, p->str, 1, 1);
      #endif

      }

      /* 2007.06.06, SJ */

      if(cfg.use_pairs == 1 && strchr(p->str, '+')) assign_spaminess(mysql, p->str, cfg, uid);
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



      /*if(p)
         free(p);*/

      p = q;
   }


   /* add a spammy token if we got a binary, eg. PDF attachment, 2007.07.02, SJ */

   if(cfg.penalize_octet_stream == 1 && (attachment_by_type(state, "application/octet-stream") == 1 || attachment_by_type(state, "application/pdf") == 1)){
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
   assign_spaminess(mysql, state.from, cfg, uid);
   addnode(B_hash, state.from, 0, 0);


   /* redesigned spaminess calculation, 2007.05.21, SJ */

   if(n_phrases > cfg.min_phrase_number){
      spaminess = sorthash(s_phrase_hash, MAX_PHRASES_TO_CHOOSE, cfg);

      /*
       * if spamicity is around 0.5 (0.4-0.6) or just a little below the spam limit,
       * check the single tokens too, then choose the value with greater deviation.
       */

      if(DEVIATION(spaminess) < cfg.min_deviation_to_use_single_tokens || (spaminess < cfg.spam_overall_limit && spaminess > cfg.use_single_tokens_min_limit)){
      #ifdef HAVE_MYSQL_TOKEN_DATABASE
         walk_hash(mysql, B_hash, cfg);
      #else
         walk_hash(B_hash, cfg);
      #endif
         spaminess2 = sorthash(shash, MAX_TOKENS_TO_CHOOSE, cfg);
      }
   }
   /* use the single tokens hash if we have not enough phrases, 2007.04.27, SJ */
   else {
   #ifdef HAVE_MYSQL_TOKEN_DATABASE
      n_tokens += walk_hash(mysql, B_hash, cfg);
   #else
      n_tokens += walk_hash(B_hash, cfg);
   #endif

      spaminess = sorthash(shash, MAX_TOKENS_TO_CHOOSE, cfg);
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

      if(n_phrases > cfg.min_phrase_number){
         spaminess = sorthash(s_phrase_hash, MAX_PHRASES_TO_CHOOSE, cfg);
      }
      else {
         if(n_tokens < 8){
         #ifdef HAVE_MYSQL_TOKEN_DATABASE
            n_tokens += walk_hash(mysql, B_hash, cfg);
         #else
            n_tokens += walk_hash(B_hash, cfg);
         #endif
         }
         spaminess = sorthash(shash, MAX_TOKENS_TO_CHOOSE, cfg);
      }

      if(DEVIATION(spaminess) < cfg.min_deviation_to_use_single_tokens || (spaminess < cfg.spam_overall_limit && spaminess > cfg.use_single_tokens_min_limit)){
         if(n_tokens < 8){
         #ifdef HAVE_MYSQL_TOKEN_DATABASE
            n_tokens += walk_hash(mysql, B_hash, cfg);
         #else
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


   if(cfg.training_mode == T_TUM && (uid > 0 || cfg.group_type == 0) && (spaminess >= cfg.spam_overall_limit || spaminess < cfg.max_junk_spamicity)){
      gettimeofday(&tv1, &tz);

      if(spaminess >= cfg.spam_overall_limit){
         n = my_walk_hash(mysql, sd, 1, cfg.mysqltokentable, tumhash, uid, T_TOE);
         snprintf(buf, MAXBUFSIZE-1, "update %s set nspam=nspam+1 WHERE uid=%ld", cfg.mysqlmisctable, uid);
      }
      else {
         n = my_walk_hash(mysql, sd, 0, cfg.mysqltokentable, tumhash, uid, T_TOE);
         snprintf(buf, MAXBUFSIZE-1, "update %s set nham=nham+1 WHERE uid=%ld", cfg.mysqlmisctable, uid);
      }

      gettimeofday(&tv2, &tz);

      syslog(LOG_PRIORITY, "%s: TUM training %ld tokens for uid: %ld %ld [ms]", spamfile, n, uid, tvdiff(tv2, tv1)/1000);

      mysql_real_query(&mysql, buf, strlen(buf));

      snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg.clapf_header_field);
   }


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
#else
double bayes_file(char *spamfile, struct session_data sdata, struct __config cfg){
#endif

   struct _state state;
   char buf[MAXBUFSIZE], ext_cmd[SMALLBUFSIZE], ifile[SMALLBUFSIZE], *p, *q;
   float spaminess, ham_from=0, spam_from=0;
   FILE *F;

   if(spamfile == NULL)
      return ERR_BAYES_NO_SPAM_FILE;

   state = init_state();


   if(strlen(cfg.gocr) > 3 || strlen(cfg.catdoc) > 3 )
      state.check_attachment = 1;


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



#ifdef HAVE_MYSQL_TOKEN_DATABASE

   /*
    * determine uid if we use a merged group, 2007.06.13, SJ
    */

   if(cfg.group_type == 1){

      if(cfg.training_mode == T_TUM && sdata.num_of_rcpt_to == 1){
         if(strlen(sdata.rcptto[0]) > 3){
            q = strchr(sdata.rcptto[0], '>');
            if(q){
               *q = '\0';
               q = strchr(sdata.rcptto[0], '<');
               if(q) snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", cfg.mysqlusertable, ++q);
            }
         }
         else if((q = getenv("FROM")))
            snprintf(buf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", cfg.mysqlusertable, q);

         if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
            while((res = mysql_store_result(&mysql))){
               row = mysql_fetch_row(res);
               if(row)
                  uid = atol(row[0]);
            }
            mysql_free_result(res);
         }

      }
      else if(sdata.num_of_rcpt_to == -1) uid = sdata.uid;

      snprintf(buf, MAXBUFSIZE-1, "SELECT SUM(nham), SUM(nspam) FROM %s WHERE uid=0 OR uid=%ld", cfg.mysqlmisctable, uid);
   }
   else {
      /* or this is a shared group */
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", cfg.mysqlmisctable);
   }


   /*
    * select the number of ham and spam messages, and return error if less than 1
    */

   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            ham_msg += atof(row[0]);
            spam_msg += atof(row[1]);
         }

         mysql_free_result(res);
      }
   }

   if(ham_msg <= 0 || spam_msg <= 0){
      free_and_print_list(state.first, 0);
      syslog(LOG_PRIORITY, "%s: %s", p, ERR_MYSQL_DATA);
      return ERR_BAYES_NO_TOKEN_FILE;
   }



   /*
    * auto whitelist test, 2007.06.21, SJ
    * it may be better to only count the users own experiment with this sender...
    * A further note: this way we will not TUM train the message if it's whitelisted
    */

   if(cfg.enable_auto_white_list == 1){
      snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE token=%llu AND  (uid=0 OR uid=%ld)", cfg.mysqltokentable, APHash(state.from), uid);

      if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
         res = mysql_store_result(&mysql);
         if(res != NULL){
            if((row = mysql_fetch_row(res))){
               if(row[0]) ham_from = atof(row[0]);
               if(row[1]) spam_from = atof(row[1]);
            }

            mysql_free_result(res);
         }
      }

   #ifdef DEBUG
      fprintf(stderr, "from: %.0f, %.0f\n", ham_from, spam_from);
   #endif

      if(ham_from > NUMBER_OF_GOOD_FROM && spam_from == 0){
         free_and_print_list(state.first, 0);
         return REAL_HAM_TOKEN_PROBABILITY;
      }
   }

#endif


#ifdef HAVE_CDB
   if(!cfg.tokensfile)
      return ERR_BAYES_NO_TOKEN_FILE;

   if(init_cdbs(cfg.tokensfile) == 0)
      return DEFAULT_SPAMICITY;
#endif


/* Query cache support */

#ifdef HAVE_QCACHE
   sd = qcache_socket(cfg.qcache_addr, cfg.qcache_port, cfg.qcache_socket);
   if(sd == -1)
      return DEFAULT_SPAMICITY;
#endif


   /* evaluate the tokens */

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   spaminess = eval_tokens(mysql, p, cfg, state);
#else
   spaminess = eval_tokens(p, cfg, state);
#endif

   free_and_print_list(state.first, 0);

#ifdef HAVE_QCACHE
   close(sd);
#endif

   /* if we shall mark the message as spam because of the embedded image */

   if(spaminess < cfg.spam_overall_limit && spaminess > cfg.max_embed_image_spamicity && has_embed_image == 1){
      return cfg.spaminess_of_embed_image;
   }


   /* if we are still unsure and the message has an image, run it through the gocr application, 2006.11.16, SJ */

   if(state.num_of_images > 0 || state.num_of_msword > 0){
      memset(ext_cmd, 0, SMALLBUFSIZE);
      snprintf(ifile, SMALLBUFSIZE-1, "%s", state.attachedfile);

      if(state.num_of_images > 0 && strlen(cfg.gocr) > 1)
         snprintf(ext_cmd, SMALLBUFSIZE-1, "%s %s/%s", cfg.gocr, cfg.workdir, state.attachedfile);
      if(state.num_of_msword > 0 && strlen(cfg.catdoc) > 1)
         snprintf(ext_cmd, SMALLBUFSIZE-1, "%s %s/%s", cfg.catdoc, cfg.workdir, state.attachedfile);

      if(strlen(ext_cmd) > 1 && spaminess < cfg.spam_overall_limit && spaminess > DEFAULT_SPAMICITY){

         state = init_state();

         F = popen(ext_cmd, "r");
         if(F){
            while(fgets(buf, MAXBUFSIZE-1, F)){
               state = parse(buf, state);
            }
            pclose(F);

         #ifdef DEBUG
            fprintf(stderr, "%s: running external program: %s . . . \n", p, ext_cmd);
         #endif

            if(cfg.verbosity >= _LOG_DEBUG)
               syslog(LOG_PRIORITY, "%s: running external check: %s", p, ext_cmd);

         #ifdef HAVE_MYSQL_TOKEN_DATABASE
            spaminess = eval_tokens(mysql, spamfile, cfg, state);
         #else
            spaminess = eval_tokens(spamfile, cfg, state);
         #endif

            free_and_print_list(state.first, 0);

         }
      }

      unlink(ifile);
   }

#ifdef HAVE_CDB
   close_cdbs(tokenscdb);
#endif

   return spaminess;
}

