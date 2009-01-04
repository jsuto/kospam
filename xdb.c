/*
 * xdb.c, 2009.01.04, SJ
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include "hash.h"
#include "score.h"
#include "cache.h"
#include <clapf.h>




void read_x_data(unsigned char *buf, int use_pair, struct node *s_phrase_hash[], struct node *shash[], struct node *s_mix[]){
   int j;
   char t[SMALLBUFSIZE];
   struct x_token *x;

   for(j=0; j<NETWORK_SEGMENT_SIZE; j++){
      x = (struct x_token*)(buf + j*sizeof(struct x_token));
      if(x->token == 0) break;

      snprintf(t, SMALLBUFSIZE-1, "%llu", x->token);

      if(DEVIATION(x->spaminess) > 0.1){
         if(use_pair == 1) addnode(s_phrase_hash, t, x->spaminess, DEVIATION(x->spaminess));
         else addnode(shash, t, x->spaminess, DEVIATION(x->spaminess));

         addnode(s_mix, t, x->spaminess, DEVIATION(x->spaminess));
      }

   }
}


int bulk_qry(int sd, struct _state *state, int use_pair, int *has_embed_image, struct __config *cfg, struct node *s_phrase_hash[], struct node *shash[], struct node *s_mix[]){
   int i=0, n;
   unsigned char buf[PKT_SIZE];
   struct _token *p;
   struct x_token X;
   struct timezone tz;
   struct timeval tv1, tv2;

   p = state->first;

   memset(buf, 0, PKT_SIZE);

   i++;

   X.token = APHash(state->from);
   memcpy(buf + (i-1)*sizeof(struct x_token), (char *)&X, sizeof(struct x_token));


   while(p != NULL){
      if(!( (use_pair == 1 && strchr(p->str, '+')) || (use_pair == 0 && !strchr(p->str, '+')) || strchr(p->str, '*') ) ){
         //printf("skipping %s\n", p->str);
         goto NEXT_ITEM;
      }

      if(cfg->penalize_embed_images == 1 && strcmp(p->str, "src+cid") == 0){
         addnode(s_phrase_hash, "EMBED*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
         addnode(shash, "EMBED*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
         addnode(s_mix, "EMBED*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
         *has_embed_image = 1;
      }

      i++;

      X.token = APHash(p->str);
      if((i % NETWORK_SEGMENT_SIZE)){
         memcpy(buf + (i-1)*sizeof(struct x_token), (char *)&X, sizeof(struct x_token));
         //printf("token: %llu\n", X.token);
      }
      else {
         memcpy(buf + (i-1)*sizeof(struct x_token), (char *)&X, sizeof(struct x_token));

         gettimeofday(&tv1, &tz);
         send(sd, buf, PKT_SIZE, 0);
         i = 0;

         n = recvtimeout2(sd, buf, PKT_SIZE, 0);
         gettimeofday(&tv2, &tz); 
         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "packet exchange in %ld [us]\n", tvdiff(tv1, tv2));
         if(n <= 0) break;

         read_x_data(buf, use_pair, s_phrase_hash, shash, s_mix);
      }

   NEXT_ITEM:
      p = p->r;
   }

   if(i > 0){ 
      gettimeofday(&tv1, &tz);
      send(sd, buf, PKT_SIZE, 0);
      n = recvtimeout2(sd, buf, PKT_SIZE, 0);
      gettimeofday(&tv2, &tz);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "packet exchange in %ld [us]\n", tvdiff(tv1, tv2));

      if(n > 0) read_x_data(buf, use_pair, s_phrase_hash, shash, s_mix);
   }

#ifdef HAVE_XFORWARD
   if(sdata->unknown_client == 1){
      addnode(s_phrase_hash, "UNKNOWN_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      addnode(shash, "UNKNOWN_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      addnode(s_mix, "UNKNOWN_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
   }
#endif



   return 1;
}


double x_spam_check(struct session_data *sdata, struct _state *state, struct __config *cfg){
   int sd;
   int has_embed_image=0, found_on_rbl=0, surbl_match=0;
   struct in_addr addr;
   struct sockaddr_in clamd_addr;
   struct timezone tz;
   struct timeval tv1, tv2;
   float spaminess;
#ifdef HAVE_RBL
   struct url *url;
   char surbl_token[MAX_TOKEN_LEN];
   int i, j;
#endif
   struct node *s_phrase_hash[MAXHASH], *shash[MAXHASH], *s_mix[MAXHASH];

   inithash(shash);
   inithash(s_phrase_hash);
   inithash(s_mix);

   if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return DEFAULT_SPAMICITY;

   clamd_addr.sin_family = AF_INET;
   clamd_addr.sin_port = htons(MY_PORT);
   inet_aton(cfg->listen_addr, &addr);
   clamd_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(clamd_addr.sin_zero), 8);

   if(connect(sd, (struct sockaddr *)&clamd_addr, sizeof(struct sockaddr)) == -1) return DEFAULT_SPAMICITY;

   /* add a spammy token if we got a binary, eg. PDF attachment, 2007.07.02, SJ */

   if(cfg->penalize_octet_stream == 1 && (attachment_by_type(*state, "application/octet-stream") == 1 || attachment_by_type(*state, "application/pdf") == 1
       || attachment_by_type(*state, "application/vnd.ms-excel") == 1
       || attachment_by_type(*state, "application/msword") == 1
       || attachment_by_type(*state, "application/rtf") == 1
       || attachment_by_type(*state, "application/x-zip-compressed") == 1)
   ){
       addnode(s_phrase_hash, "OCTET_STREAM*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
       addnode(shash, "OCTET_STREAM*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
       addnode(s_mix, "OCTET_STREAM*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
   }

   /* add penalty for images, 2007.07.02, SJ */

   if(cfg->penalize_images == 1 && attachment_by_type(*state, "image/") == 1){
       addnode(s_phrase_hash, "IMAGE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
       addnode(shash, "IMAGE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
       addnode(s_mix, "IMAGE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
   }


   if(state->n_subject_token == 0){
      addnode(s_phrase_hash, "NO_SUBJECT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      addnode(shash, "NO_SUBJECT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      addnode(s_mix, "NO_SUBJECT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
   }

   bulk_qry(sd, state, 1, &has_embed_image, cfg, s_phrase_hash, shash, s_mix);
   spaminess = calc_score_chi2(s_phrase_hash, cfg);
   if(cfg->debug == 1) fprintf(stderr, "phrase: %.4f\n", spaminess);

   if(spaminess >= cfg->spam_overall_limit || spaminess <= cfg->max_ham_spamicity) goto END_OF_EVAL;

   #ifdef HAVE_RBL
      if(strlen(cfg->rbl_domain) > 3){
         gettimeofday(&tv1, &tz);
         found_on_rbl = rbl_list_check(cfg->rbl_domain, state->ip, cfg->debug);
         gettimeofday(&tv2, &tz);

         if(cfg->debug == 1) fprintf(stderr, "rbl check took %ld ms\n", tvdiff(tv2, tv1)/1000);
         if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: rbl check took %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

         for(i=0; i<found_on_rbl; i++){
            snprintf(surbl_token, MAX_TOKEN_LEN-1, "RBL%d*%s", i, state->ip);
            addnode(s_phrase_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            addnode(shash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            addnode(s_mix, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
         }
      }
   #endif

   #ifdef HAVE_RBL
      if(state->urls){
         url = state->urls;

         while(url){
            gettimeofday(&tv1, &tz);
            i = rbl_list_check(cfg->surbl_domain, url->url_str+4, cfg->debug);
            gettimeofday(&tv2, &tz);

            if(cfg->debug == 1) fprintf(stderr, "surbl check for %s (%d) took %ld ms\n", url->url_str+4, i, tvdiff(tv2, tv1)/1000);

            surbl_match += i;

            for(j=0; j<i; j++){
               snprintf(surbl_token, MAX_TOKEN_LEN-1, "SURBL%d*%s", j, url->url_str+4);
               addnode(s_phrase_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
               addnode(shash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
               addnode(s_mix, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            }

            url = url->r;
         }
      }
   #endif

   bulk_qry(sd, state, 0, &has_embed_image, cfg, s_phrase_hash, shash, s_mix);
   spaminess = calc_score_chi2(s_mix, cfg);
   if(cfg->debug == 1) fprintf(stderr, "mix: %.4f\n", spaminess);

END_OF_EVAL:
   clearhash(shash);
   clearhash(s_phrase_hash);
   clearhash(s_mix);

   close(sd);

   if(spaminess > cfg->max_ham_spamicity && spaminess < cfg->spam_overall_limit)
      spaminess = apply_fixes(spaminess, found_on_rbl, surbl_match, has_embed_image, state->base64_text, state->c_shit, state->l_shit, cfg);

   return spaminess;
}




