/*
 * hash2.c, 2010.05.17, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include "misc.h"
#include "hash.h"
#include "parser.h"
#include "cfg.h"
#include "config.h"
#include "defs.h"


/*
 * add penalties
 */

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


   /* if no valid Subject: line */

   if(state->n_subject_token == 0)
      addnode(state->token_hash, "NO_SUBJECT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));


   if(strcmp(state->hostname, "unknown") == 0 && sdata->Nham > NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED)
      addnode(state->token_hash, "UNKNOWN_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));


   /* blackhole IP-address info */
   if(sdata->trapped_client == 1)
      addnode(state->token_hash, "TRAPPED_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(sdata->tre == '+')
      addnode(state->token_hash, "ZOMBIE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

}


void check_lists(struct session_data *sdata, struct _state *state, int *found_on_rbl, int *surbl_match, struct __config *cfg){
#ifdef HAVE_RBL
   int i, j;
   char surbl_token[MAX_TOKEN_LEN];
   struct url *url;
   struct timezone tz;
   struct timeval tv1, tv2;


   /* consult blacklists about the IPv4 address connecting to us */

   if(strlen(cfg->rbl_domain) > 3){
      gettimeofday(&tv1, &tz);
      *found_on_rbl = isIPv4AddressOnRBL(state->ip, cfg->rbl_domain);
      gettimeofday(&tv2, &tz);

      if(cfg->debug == 1) printf("rbl check took %ld ms\n", tvdiff(tv2, tv1)/1000);
      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: rbl check took %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

      for(i=0; i<*found_on_rbl; i++){
         snprintf(surbl_token, MAX_TOKEN_LEN-1, "RBL%d*%s", i, state->ip);
         addnode(state->token_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      }
   }


   /* consult URL blacklists */

   if(state->urls && strlen(cfg->surbl_domain) > 4){
      url = state->urls;

      while(url){
         if(countCharacterInBuffer(url->url_str+4, '.') > 0){
            gettimeofday(&tv1, &tz);
            i = isURLOnRBL(url->url_str+4, cfg->surbl_domain);
            gettimeofday(&tv2, &tz);

            if(cfg->debug == 1) printf("surbl check for %s (%d) took %ld ms\n", url->url_str+4, i, tvdiff(tv2, tv1)/1000);
            if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: surbl check took %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

            *surbl_match += i;

            for(j=0; j<i; j++){
               snprintf(surbl_token, MAX_TOKEN_LEN-1, "SURBL%d*%s", j, url->url_str+4);
               addnode(state->token_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            }
         }

         url = url->r;
      }
   }

#endif
}


