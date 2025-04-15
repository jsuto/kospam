/*
 * antispam.c, SJ
 */

#include <kospam.h>

void add_penalties(struct session_data *sdata, struct parser_state *state, struct config *cfg){

   if(cfg->penalize_octet_stream == 1 && state->has_octet_stream_attachment == 1)
       addnode(state->token_hash, "OCTET_STREAM*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(cfg->penalize_images == 1 && state->has_image_attachment == 1)
       addnode(state->token_hash, "IMAGE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(state->n_subject_token == 0)
      addnode(state->token_hash, "NO_SUBJECT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(sdata->trapped_client == 1)
      addnode(state->token_hash, "TRAPPED_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

   if(state->tre == '+')
      addnode(state->token_hash, "ZOMBIE*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));

}


int check_spam(struct session_data *sdata, MYSQL *conn, struct parser_state *state, struct data *data, struct config *cfg){
   char tmpbuf[SMALLBUFSIZE];
   struct timezone tz;
   struct timeval tv1, tv2;

   sdata->spaminess = DEFAULT_SPAMICITY;

   int spaminessbuf_size_left = sizeof(sdata->spaminessbuf)-2;
   snprintf(sdata->spaminessbuf, spaminessbuf_size_left, "%s%s\r\n", HEADER_KOSPAM_WATERMARK, sdata->ttmpfile);
   spaminessbuf_size_left -= strlen(sdata->spaminessbuf);

   /*
    * Handle a training request
    */

   if(state->training_request == 1){
      gettimeofday(&tv1, &tz);
      do_training(sdata, state, conn, cfg);
      gettimeofday(&tv2, &tz);
      sdata->__training += tvdiff(tv2, tv1);

      return MESSAGE_DISCARD;
   }


   /*
    * skip the antispam engine if the email comes from mynetwork
    */

   // TODO
   if(sdata->mynetwork == 1){
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: mynetwork: %s", sdata->ttmpfile, state->ip);
      return MESSAGE_OK;
   }


   /*
    * check sender address on the per user whitelist, then blacklist
    */

   if (check_email_against_list(conn, SQL_WHITE_LIST, state->envelope_from)) {
      sdata->spaminess = 0.01;
      syslog(LOG_PRIORITY, "INFO: %s: sender %s found on whitelist", sdata->ttmpfile, state->envelope_from);
      return MESSAGE_OK;
   }


   if (check_email_against_list(conn, SQL_BLACK_LIST, state->envelope_from)) {
      sdata->spaminess = 0.99;
      syslog(LOG_PRIORITY, "INFO: %s: sender %s found on blacklist", sdata->ttmpfile, state->envelope_from);
      return MESSAGE_OK;
   }


   /*
    * check if sender host is trapped on a minefield
    */

   gettimeofday(&tv1, &tz);
   sdata->trapped_client = is_sender_on_minefield(conn, state->ip);
   gettimeofday(&tv2, &tz);
   sdata->__minefield += tvdiff(tv2, tv1);


   /*
    * run zombie test
    */

   if (data->n_regex) {
      check_zombie_sender(state, data, cfg);

      if(state->tre == '+' && cfg->message_from_a_zombie > 0){
         sdata->spaminess = 0.99;

         if(cfg->message_from_a_zombie == 1){
            syslog(LOG_PRIORITY, "INFO: %s: marking message from a zombie as spam", sdata->ttmpfile);
            return MESSAGE_OK;
         }

         if(cfg->message_from_a_zombie == 2){
            syslog(LOG_PRIORITY, "INFO: %s: dropping message from a zombie (%s) as spam", sdata->ttmpfile, state->hostname);
            return MESSAGE_DISCARD;
         }
      }
   }


   /*
    * if the From: line contains any of our domain names listed in mydomains
    * and we are absolutely sure that no valid email comes from outside with
    * our domainname in the email header From: line, then we can condemn the
    * message.
    */

   // TODO
   if(sdata->from_address_in_mydomain == 1 && cfg->mydomains_from_outside_is_spam == 1){
      sdata->spaminess = 0.99;
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: %s matches %s", sdata->ttmpfile, state->from, cfg->mydomains);
      return MESSAGE_OK;
   }


   /*
    * some MTAs strip our signo from the bounce. So if we would raise the spaminess
    * then we may commit a false positive. Thus in case of a missing signo, let
    * the statistical analysis decide the fate of a dummy bounce message. 2009.01.20, SJ
    */

   // TODO
   if(sdata->need_signo_check == 1){
      if(state->found_our_signo == 1){
         syslog(LOG_PRIORITY, "%s: bounce message, found our signo", sdata->ttmpfile);
         return MESSAGE_OK;
      }
      else
         syslog(LOG_PRIORITY, "%s: looks like a bounce, but our signo is missing", sdata->ttmpfile);
   }



   if(cfg->use_antispam == 1 && (cfg->max_message_size_to_filter == 0 || sdata->tot_len < cfg->max_message_size_to_filter || state->n_token < cfg->max_number_of_tokens_to_filter) ){

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running statistical test", sdata->ttmpfile);

      gettimeofday(&tv1, &tz);
      sdata->spaminess = run_statistical_check(sdata, state, conn, cfg);
      gettimeofday(&tv2, &tz);
      sdata->__as = tvdiff(tv2, tv1);



      /* training a blackhole message as spam, if we have to */

      if(sdata->blackhole == 1 && sdata->spaminess < 0.99){
         gettimeofday(&tv1, &tz);
         train_message(state, "nspam", cfg);
         gettimeofday(&tv2, &tz);
         sdata->__training = tvdiff(tv2, tv1);

         syslog(LOG_PRIORITY, "%s: training on a blackhole message", sdata->ttmpfile);
      }

      // TODO: revise if we could replace strncat
      snprintf(tmpbuf, SMALLBUFSIZE-1, "%s%.4f\r\n", cfg->clapf_header_field, sdata->spaminess);
      strncat(sdata->spaminessbuf, tmpbuf, spaminessbuf_size_left);
      spaminessbuf_size_left -= strlen(tmpbuf);

      if(sdata->spaminess >= cfg->spam_overall_limit) {
         strncat(sdata->spaminessbuf, cfg->clapf_spam_header_field, spaminessbuf_size_left);
         spaminessbuf_size_left -= strlen(cfg->clapf_spam_header_field);
      }
   }
   else {
      syslog(LOG_PRIORITY, "%s: skipping spam check, size: %d/%d, tokens: %d/%d", sdata->ttmpfile, sdata->tot_len, cfg->max_message_size_to_filter, state->n_token, cfg->max_number_of_tokens_to_filter);
   }


   if(cfg->update_tokens == 1 && state->n_token > 3){
      gettimeofday(&tv1, &tz);
      int utokens = update_token_dates(state, cfg);
      gettimeofday(&tv2, &tz);
      sdata->__update = tvdiff(tv2, tv1);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: updated %d/%d tokens", sdata->ttmpfile, utokens, state->n_token);
   }

   chop_newlines(sdata->spaminessbuf, strlen(sdata->spaminessbuf));

   return MESSAGE_OK;
}
