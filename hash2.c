/*
 * hash2.c, 2009.01.04, SJ
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

void add_penalties(struct session_data *sdata, struct _state state, struct node *xhash[], struct __config *cfg){

   /* add a spammy token if we got a binary, eg. PDF attachment, 2007.07.02, SJ */

   if(cfg->penalize_octet_stream == 1 && (attachment_by_type(state, "application/octet-stream") == 1 || attachment_by_type(state, "application/pdf") == 1
       || attachment_by_type(state, "application/vnd.ms-excel") == 1
       || attachment_by_type(state, "application/msword") == 1
       || attachment_by_type(state, "application/rtf") == 1
       || attachment_by_type(state, "application/x-zip-compressed") == 1)
   ){
       addnode(xhash, "OCTET_STREAM*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
   }

   /* add penalty for images, 2007.07.02, SJ */

   if(cfg->penalize_images == 1 && attachment_by_type(state, "image/") == 1)
       addnode(xhash, "IMAGE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));


   /* if no valid Subject: line */

   if(state.n_subject_token == 0)
      addnode(xhash, "NO_SUBJECT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));


#ifdef HAVE_XFORWARD
   if(sdata->unknown_client == 1 && sdata->Nham > NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED)
      addnode(xhash, "UNKNOWN_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
#endif

}


void check_lists(struct session_data *sdata, struct _state *state, struct node *xhash[], int *found_on_rbl, int *surbl_match, struct __config *cfg){
#ifdef HAVE_RBL
   int i, j;
   char surbl_token[MAX_TOKEN_LEN];
   struct url *url;
   struct timezone tz;
   struct timeval tv1, tv2;


   /* consult blacklists about the IPv4 address connecting to us */

   if(strlen(cfg->rbl_domain) > 3){
      gettimeofday(&tv1, &tz);
      *found_on_rbl = rbl_list_check(cfg->rbl_domain, state->ip, cfg->debug);
      gettimeofday(&tv2, &tz);

      if(cfg->debug == 1) fprintf(stderr, "rbl check took %ld ms\n", tvdiff(tv2, tv1)/1000);
      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: rbl check took %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

      for(i=0; i<*found_on_rbl; i++){
         snprintf(surbl_token, MAX_TOKEN_LEN-1, "RBL%d*%s", i, state->ip);
         addnode(xhash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      }
   }


   /* consult URL blacklists */

   if(state->urls){
      url = state->urls;

      while(url){
         gettimeofday(&tv1, &tz);
         i = rbl_list_check(cfg->surbl_domain, url->url_str+4, cfg->debug);
         gettimeofday(&tv2, &tz);

         if(cfg->debug == 1) fprintf(stderr, "surbl check for %s (%d) took %ld ms\n", url->url_str+4, i, tvdiff(tv2, tv1)/1000);
         if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: surbl check took %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

         *surbl_match += i;

         for(j=0; j<i; j++){
            snprintf(surbl_token, MAX_TOKEN_LEN-1, "SURBL%d*%s", j, url->url_str+4);
            addnode(xhash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
         }

         url = url->r;
      }
   }

#endif
}


