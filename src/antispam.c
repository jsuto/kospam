/*
 * antispam.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <clapf.h>


void add_penalties(struct session_data *sdata, struct __state *state, struct __config *cfg){

   if(cfg->penalize_octet_stream == 1 && has_octet_stream(state) == 1)
       addnode(state->token_hash, "OCTET_STREAM*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(cfg->penalize_images == 1 && has_image_attachment(state) == 1)
       addnode(state->token_hash, "IMAGE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(state->n_subject_token == 0)
      addnode(state->token_hash, "NO_SUBJECT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(strcmp(sdata->hostname, "unknown") == 0)
      addnode(state->token_hash, "UNKNOWN_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(sdata->trapped_client == 1)
      addnode(state->token_hash, "TRAPPED_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(sdata->tre == '+')
      addnode(state->token_hash, "ZOMBIE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

}


int check_spam(struct session_data *sdata, struct __state *state, struct __data *data, char *fromemail, char *rcpttoemail, struct __config *cfg, struct __config *my_cfg){
   char *p, tmpbuf[SMALLBUFSIZE];
   struct timezone tz;
   struct timeval tv1, tv2;

   sdata->spaminess = DEFAULT_SPAMICITY;

   snprintf(sdata->spaminessbuf, MAXBUFSIZE-1, "%s%s\r\n", cfg->clapf_header_field, sdata->ttmpfile);

   /*
    * do training
    */

   if(sdata->training_request == 1){

      /* get user from 'MAIL FROM:', 2008.10.25, SJ */

      gettimeofday(&tv1, &tz);
      get_user_data_from_email(sdata, fromemail, cfg);
      gettimeofday(&tv2, &tz);
      sdata->__user += tvdiff(tv2, tv1);

      /*
       * If not found, then try to get it from the RCPT TO address.
       *
       * This may happen if your email address is xy@mail.domain.com,
       * but xy@domain.com is set in your email client application as
       * your email address.
       * In this case send training emails to xy+spam@mail.domain.com
       */

      if(sdata->name[0] == 0){
         gettimeofday(&tv1, &tz);
         get_user_data_from_email(sdata, rcpttoemail, cfg);
         gettimeofday(&tv2, &tz);
         sdata->__user += tvdiff(tv2, tv1);
      }

      if(sdata->policy_group > 0) get_policy(sdata, cfg, my_cfg);

      gettimeofday(&tv1, &tz);
      do_training(sdata, state, rcpttoemail, my_cfg);
      gettimeofday(&tv2, &tz);
      sdata->__training += tvdiff(tv2, tv1);

      return DISCARD;
   }


   /*
    * skip the antispam engine if the email comes from mynetwork
    */

   if(sdata->mynetwork == 1){
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: mynetwork: %s", sdata->ttmpfile, sdata->ip);
      return OK;
   }


   /*
    * get per user settings and policy
    */

   if(get_user_data_from_email(sdata, rcpttoemail, cfg) == 0){
      p = strchr(rcpttoemail, '@');
      if(p) get_user_data_from_email(sdata, p, cfg);
   }

   if(sdata->policy_group > 0) get_policy(sdata, cfg, my_cfg);


   /*
    * check sender address on the per user whitelist, then blacklist
    */

   if(is_item_on_list(fromemail, sdata->whitelist, "") == 1){
      syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata->ttmpfile, fromemail);
      return OK;
   }


   if(is_item_on_list(fromemail, sdata->blacklist, "") == 1){
      sdata->spaminess = 0.99;
      syslog(LOG_PRIORITY, "%s: sender (%s) found on blacklist", sdata->ttmpfile, fromemail);
      return DISCARD;
   }


   /*
    * check if sender host is trapped on a minefield
    */

   gettimeofday(&tv1, &tz);
   is_sender_on_minefield(sdata, sdata->ip);
   gettimeofday(&tv2, &tz);
   sdata->__minefield += tvdiff(tv2, tv1);


   /*
    * run zombie test
    */

   check_zombie_sender(sdata, data, my_cfg);

   if(sdata->tre == '+' && my_cfg->message_from_a_zombie > 0){
      sdata->spaminess = 0.99;

      if(my_cfg->message_from_a_zombie == 1){
         syslog(LOG_PRIORITY, "%s: marking message from a zombie as spam", sdata->ttmpfile);
         return OK;
      }

      if(my_cfg->message_from_a_zombie == 2){
         syslog(LOG_PRIORITY, "%s: dropping message from a zombie (%s) as spam", sdata->ttmpfile, sdata->hostname);
         return DISCARD;
      }
   }


   /*
    * sometimes spammers try to send their crap in very few smtp sessions
    * including lots of recipients in a single smtp session.
    * If the spammer included at least max_number_of_recipients_in_ham+1 recipients
    * in the RCPT TO commands, mark his messages as spam
    */

    if(sdata->num_of_rcpt_to > my_cfg->max_number_of_recipients_in_ham){
       sdata->spaminess = 0.99;
       if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: marking message for %s as spam, reason: too many recipients (%d/%d)", sdata->ttmpfile, rcpttoemail, sdata->num_of_rcpt_to, my_cfg->max_number_of_recipients_in_ham);
       return OK;
    }


   /*
    * if the From: line contains any of our domain names listed in mydomains
    * and we are absolutely sure that no valid email comes from outside with
    * our domainname in the email header From: line, then we can condemn the
    * message.
    */

   if(sdata->from_address_in_mydomain == 1 && my_cfg->mydomains_from_outside_is_spam == 1){
      sdata->spaminess = 0.99;
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: %s matches %s", sdata->ttmpfile, state->from, my_cfg->mydomains);
      return OK;
   }


   /*
    * some MTAs strip our signo from the bounce. So if we would raise the spaminess
    * then we may commit a false positive. Thus in case of a missing signo, let
    * the statistical analysis decide the fate of a dummy bounce message. 2009.01.20, SJ
    */

   if(sdata->need_signo_check == 1){
      if(state->found_our_signo == 1){
         syslog(LOG_PRIORITY, "%s: bounce message, found our signo", sdata->ttmpfile);
         return OK;
      }
      else
         syslog(LOG_PRIORITY, "%s: looks like a bounce, but our signo is missing", sdata->ttmpfile);
   }



   if(my_cfg->use_antispam == 1 && (my_cfg->max_message_size_to_filter == 0 || sdata->tot_len < my_cfg->max_message_size_to_filter || state->n_token < my_cfg->max_number_of_tokens_to_filter) ){

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running statistical test", sdata->ttmpfile);

      if(sdata->blackhole == 1) sdata->gid = 0; // fix gid if it's a blackhole request

      if(cfg->group_type == GROUP_SHARED) sdata->gid = 0;

      gettimeofday(&tv1, &tz);
      sdata->spaminess = run_statistical_check(sdata, state, my_cfg);
      gettimeofday(&tv2, &tz);
      sdata->__as = tvdiff(tv2, tv1);



      /* skip TUM training on a blackhole message, unless it may learn a missed spam as a good email */

      if(
         (sdata->blackhole == 0 && my_cfg->training_mode == T_TUM && ( (sdata->spaminess >= my_cfg->spam_overall_limit && sdata->spaminess < 0.99) || (sdata->spaminess < my_cfg->max_ham_spamicity && sdata->spaminess > 0.1) ))
      )
      {

         char s[SMALLBUFSIZE];
         if(sdata->spaminess >= my_cfg->spam_overall_limit){
            snprintf(s, sizeof(s)-1, "nspam");
            syslog(LOG_PRIORITY, "%s: TUM training a spam", sdata->ttmpfile);
         }
         else {
            snprintf(s, sizeof(s)-1, "nham");
            syslog(LOG_PRIORITY, "%s: TUM training a ham", sdata->ttmpfile);
         }

         snprintf(tmpbuf, sizeof(tmpbuf)-1, "%sTUM\r\n", cfg->clapf_header_field);
         strncat(sdata->spaminessbuf, tmpbuf, MAXBUFSIZE-1);

         gettimeofday(&tv1, &tz);
         train_message(sdata, state, s, my_cfg);
         gettimeofday(&tv2, &tz);
         sdata->__training = tvdiff(tv2, tv1);
      }



      /* training a blackhole message as spam, if we have to */

      if(sdata->blackhole == 1 && sdata->spaminess < 0.99){

         snprintf(tmpbuf, sizeof(tmpbuf)-1, "%sTUM on minefield\r\n", cfg->clapf_header_field);
         strncat(sdata->spaminessbuf, tmpbuf, MAXBUFSIZE-1);

         gettimeofday(&tv1, &tz);
         train_message(sdata, state, "nspam", my_cfg);
         gettimeofday(&tv2, &tz);
         sdata->__training = tvdiff(tv2, tv1);

         syslog(LOG_PRIORITY, "%s: training on a blackhole message", sdata->ttmpfile);
      }

      snprintf(tmpbuf, SMALLBUFSIZE-1, "%s%.4f\r\n", cfg->clapf_header_field, sdata->spaminess);
      strncat(sdata->spaminessbuf, tmpbuf, MAXBUFSIZE-1);

   }
   else {
      syslog(LOG_PRIORITY, "%s: skipping spam check, size: %d/%d, tokens: %d/%d", sdata->ttmpfile, sdata->tot_len, my_cfg->max_message_size_to_filter, state->n_token, my_cfg->max_number_of_tokens_to_filter);
   }


   if(cfg->update_tokens == 1 && state->n_token > 3){
      gettimeofday(&tv1, &tz);
      int utokens = update_token_dates(state, cfg);
      gettimeofday(&tv2, &tz);
      sdata->__update = tvdiff(tv2, tv1);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: updated %d/%d tokens", sdata->ttmpfile, utokens, state->n_token);
   }

   trim_buffer(sdata->spaminessbuf);

   return OK;
}
