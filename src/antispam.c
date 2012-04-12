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


int processMessage(struct session_data *sdata, struct _state *sstate, struct __data *data, char *rcpttoemail, char *fromemail, struct __config *cfg, struct __config *my_cfg){
   int is_spam = 0, utokens;
   char reason[SMALLBUFSIZE], resp[MAXBUFSIZE], tmpbuf[SMALLBUFSIZE], trainbuf[SMALLBUFSIZE], whitelistbuf[SMALLBUFSIZE];
   struct timezone tz;
   struct timeval tv1, tv2;

   memset(sdata->acceptbuf, 0, SMALLBUFSIZE);

   strcpy(resp, "-");


#ifdef HAVE_USERS
   getUserFromEmailAddress(sdata, data, rcpttoemail, cfg);
#endif

#ifdef HAVE_POLICY
   if(sdata->policy_group > 0) getPolicySettings(sdata, data, cfg, my_cfg);
#endif

   memset(sdata->spaminessbuf, 0, MAXBUFSIZE);

   /* create a default spaminess buffer containing the clapf id */
   snprintf(sdata->spaminessbuf, MAXBUFSIZE-1, "%s%s\r\n", cfg->clapf_header_field, sdata->ttmpfile);

   if(sdata->rav == AVIR_VIRUS){

      snprintf(tmpbuf, SMALLBUFSIZE-1, "%sVIRUS\r\n", cfg->clapf_header_field);
      strncat(sdata->spaminessbuf, tmpbuf, MAXBUFSIZE-1);

      saveMessageToQueue(sdata, sdata->spaminess, my_cfg);

      if(my_cfg->deliver_infected_email == 1) return OK;

      if(my_cfg->silently_discard_infected_email == 1)
         snprintf(sdata->acceptbuf, SMALLBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata->ttmpfile, rcpttoemail);
      else
         snprintf(sdata->acceptbuf, SMALLBUFSIZE-1, "550 %s %s\r\n", sdata->ttmpfile, rcpttoemail);

      return DISCARD;
   }


   memset(reason, 0, SMALLBUFSIZE);
   memset(trainbuf, 0, SMALLBUFSIZE);
   memset(whitelistbuf, 0, SMALLBUFSIZE);


#ifdef HAVE_TRE
   checkZombieSender(sdata, data, sstate, cfg);
   if(sdata->tre == '+'){
      snprintf(sdata->acceptbuf, SMALLBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata->ttmpfile, rcpttoemail);

      if(my_cfg->message_from_a_zombie == 1){
         sdata->spaminess = 0.99;
         strncat(sdata->spaminessbuf, cfg->clapf_spam_header_field, MAXBUFSIZE-1);

         return OK;
      }

      if(my_cfg->message_from_a_zombie == 2){
         syslog(LOG_PRIORITY, "%s: dropping message from a zombie as spam", sdata->ttmpfile);

         return DISCARD;
      }
   }
#endif



   if(sdata->training_request == 1){

   #ifdef HAVE_USERS
      /* get user from 'MAIL FROM:', 2008.10.25, SJ */

      gettimeofday(&tv1, &tz);
      getUserFromEmailAddress(sdata, data, fromemail, cfg);
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
          getUserFromEmailAddress(sdata, data, rcpttoemail, cfg);
          gettimeofday(&tv2, &tz);
          sdata->__user += tvdiff(tv2, tv1);
       }

    #ifdef HAVE_POLICY
       if(sdata->policy_group > 0) getPolicySettings(sdata, data, cfg, my_cfg);
    #endif

       /* if still not found, then let this email slip through clapf, 2009.03.12, SJ */

       if(sdata->name[0] == 0) return OK;
    #ifndef SPAMC_EMUL
       gettimeofday(&tv1, &tz);
       do_training(sdata, sstate, rcpttoemail, &(sdata->acceptbuf[0]), my_cfg);
       gettimeofday(&tv2, &tz);
       sdata->__training += tvdiff(tv2, tv1);
    #endif

       return DISCARD;

    #else

       /*
        * if you have not specified --with-userdb=.... then we don't know
        * much about the recipient, so you have to do the training with
        * spamdrop
        */

       return OK;
    #endif
    }


   /* skip the antispam engine if the email comes from mynetwork */

   if(sdata->mynetwork == 1){
      snprintf(tmpbuf, SMALLBUFSIZE-1, "%smynetwork\r\n", cfg->clapf_header_field);
      strncat(sdata->spaminessbuf, tmpbuf, MAXBUFSIZE-1);

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: mynetwork: %s", sdata->ttmpfile, sdata->client_addr);

      return OK;
   }


   if(sdata->blackhole == 1) my_cfg->use_antispam = 1;



#ifdef HAVE_WHITELIST
   getUsersWBL(sdata, data, cfg);
#endif

#ifdef HAVE_WHITELIST
   if(isEmailAddressOnList(sdata->whitelist, sdata->ttmpfile, fromemail, cfg) == 1){
      syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata->ttmpfile, fromemail);
      snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on whitelist\r\n", cfg->clapf_header_field);
      goto END_OF_TRAINING;
   }
#endif

   /*
    * sometimes spammers try to send their crap in very few smtp sessions
    * including lots of recipients in a single smtp session.
    * If the spammer included at least max_number_of_recipients_in_ham+1 recipients
    * in the RCPT TO commands, mark his messages as spam
    */

   if(sdata->num_of_rcpt_to > cfg->max_number_of_recipients_in_ham){
      sdata->spaminess = 0.99;
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: marking message for %s as spam, reason: too many recipients (%d/%d)", sdata->ttmpfile, rcpttoemail, sdata->num_of_rcpt_to, cfg->max_number_of_recipients_in_ham);
      goto END_OF_TRAINING;
   }


   /*
    * run statistical antispam check
    */

   if(my_cfg->use_antispam == 1 && (my_cfg->max_message_size_to_filter == 0 || sdata->tot_len < my_cfg->max_message_size_to_filter || sstate->n_token < my_cfg->max_number_of_tokens_to_filter) ){

   #ifdef HAVE_BLACKHOLE
      gettimeofday(&tv1, &tz);
      is_sender_on_minefield(sdata, sdata->client_addr, cfg);
      gettimeofday(&tv2, &tz);
      sdata->__minefield += tvdiff(tv2, tv1);
   #endif


   #ifdef SPAMC_EMUL
      gettimeofday(&tv1, &tz);
      int rc = spamc_emul(sdata->ttmpfile, sdata->tot_len, cfg);
      gettimeofday(&tv2, &tz);
      sdata->__as = tvdiff(tv2, tv1);

      if(rc == 1){
         sdata->spaminess = 0.99;
         strncat(sdata->spaminessbuf, cfg->clapf_spam_header_field, MAXBUFSIZE-1);
      }

      if(cfg->verbosity >= _LOG_INFO){
         snprintf(tmpbuf, SMALLBUFSIZE-1, "%s%.0f ms\r\n", cfg->clapf_header_field, sdata->__as/1000.0);
         strncat(sdata->spaminessbuf, tmpbuf, MAXBUFSIZE-1);
      }

      return OK;
   #endif

   #ifdef HAVE_MYDB
      gettimeofday(&tv1, &tz);
      sdata->spaminess = bayes_file(sdata, sstate, cfg);
      gettimeofday(&tv2, &tz);
      sdata->__as = tvdiff(tv2, tv1);

      goto END_OF_TRAINING;
   #endif

      /*
       * some MTAs strip our signo from the bounce. So if we would raise the spaminess
       * then we may commit a false positive. Thus in case of a missing signo, let
       * the statistical analysis decide the fate of a dummy bounce message. 2009.01.20, SJ
       */

      if(sdata->need_signo_check == 1){
         if(sstate->found_our_signo == 1){
            syslog(LOG_PRIORITY, "%s: bounce message, found our signo", sdata->ttmpfile);
            goto END_OF_TRAINING;
         }
         else
            syslog(LOG_PRIORITY, "%s: looks like a bounce, but our signo is missing", sdata->ttmpfile);
      }


   #ifdef HAVE_BLACKLIST
      if(isEmailAddressOnList(sdata->blacklist, sdata->ttmpfile, fromemail, cfg) == 1){
         syslog(LOG_PRIORITY, "%s: sender (%s) found on blacklist", sdata->ttmpfile, fromemail);
         snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on blacklist\r\n", cfg->clapf_header_field);

         snprintf(sdata->acceptbuf, SMALLBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata->ttmpfile, rcpttoemail);

         sdata->spaminess = 0.99;

         return DISCARD;
      }
   #endif

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running Bayesian test", sdata->ttmpfile);

      /* fix gid if it's a blackhole request */
      if(sdata->blackhole == 1) sdata->gid = 0;


      gettimeofday(&tv1, &tz);
      sdata->spaminess = bayes_file(sdata, sstate, my_cfg);
      gettimeofday(&tv2, &tz);
      sdata->__as = tvdiff(tv2, tv1);

      if(sdata->spaminess > 0.9999) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg->clapf_header_field, MSG_ABSOLUTELY_SPAM);

      /* discard any training in debug mode */
      if(cfg->debug == 1) goto END_OF_TRAINING;


      /* skip TUM training on a blackhole message, unless it may learn a missed spam as a good email */

      if(
         (sdata->blackhole == 0 && my_cfg->training_mode == T_TUM && ( (sdata->spaminess >= my_cfg->spam_overall_limit && sdata->spaminess < 0.99) || (sdata->spaminess < my_cfg->max_ham_spamicity && sdata->spaminess > 0.1) )) ||
         (my_cfg->initial_1000_learning == 1 && (sdata->Nham < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED || sdata->Nspam < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED))
      )
      {

         if(sdata->spaminess >= my_cfg->spam_overall_limit){
            is_spam = 1;
            syslog(LOG_PRIORITY, "%s: TUM training a spam", sdata->ttmpfile);
         }
         else {
            is_spam = 0;
            syslog(LOG_PRIORITY, "%s: TUM training a ham", sdata->ttmpfile);
         }

         snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg->clapf_header_field);

         gettimeofday(&tv1, &tz);
         trainMessage(sdata, sstate, 1, is_spam, T_TOE, my_cfg);
         gettimeofday(&tv2, &tz);
         sdata->__training = tvdiff(tv2, tv1);
      }


      /* training a blackhole message as spam, if we have to */

      if(sdata->blackhole == 1){
         snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg->clapf_header_field, MSG_BLACKHOLED);

         if(sdata->spaminess < 0.99){
            gettimeofday(&tv1, &tz);

            trainMessage(sdata, sstate, MAX_ITERATIVE_TRAIN_LOOPS, 1, T_TOE, my_cfg);

            gettimeofday(&tv2, &tz);
            sdata->__training = tvdiff(tv2, tv1);

            snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM on blackhole\r\n", cfg->clapf_header_field);
            syslog(LOG_PRIORITY, "%s: training on a blackhole message", sdata->ttmpfile);

         }
      }



      if(cfg->update_tokens == 1){
         gettimeofday(&tv1, &tz);

         utokens = updateTokenTimestamps(sdata, sstate->token_hash);

         gettimeofday(&tv2, &tz);
         sdata->__update = tvdiff(tv2, tv1);

         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: updated %d/%ld tokens", sdata->ttmpfile, utokens, sstate->n_token);
      } 


   END_OF_TRAINING:

      if(sdata->uid > 0){
         gettimeofday(&tv1, &tz);

         saveMessageToQueue(sdata, sdata->spaminess, my_cfg);

         gettimeofday(&tv2, &tz);
         sdata->__store = tvdiff(tv2, tv1);
      }


      if(cfg->verbosity >= _LOG_INFO){
         snprintf(tmpbuf, SMALLBUFSIZE-1, "%s%ld ms\r\n%s%s", cfg->clapf_header_field, (long) (sdata->__parsed+sdata->__av+sdata->__user+sdata->__policy+sdata->__minefield+sdata->__as+sdata->__training+sdata->__update+sdata->__store)/1000, reason, whitelistbuf);
         strncat(sdata->spaminessbuf, tmpbuf, MAXBUFSIZE-1);
      }

      snprintf(tmpbuf, SMALLBUFSIZE-1, "%s%.4f\r\n%s", cfg->clapf_header_field, sdata->spaminess, trainbuf);
      strncat(sdata->spaminessbuf, tmpbuf, MAXBUFSIZE-1);

      if(sdata->spaminess >= cfg->spam_overall_limit){
         strncat(sdata->spaminessbuf, cfg->clapf_spam_header_field, MAXBUFSIZE-1);
      }

      if(sdata->spaminess >= cfg->possible_spam_limit && sdata->spaminess < cfg->spam_overall_limit && strlen(cfg->clapf_possible_spam_header_field) > 5){
         strncat(sdata->spaminessbuf, cfg->clapf_possible_spam_header_field, MAXBUFSIZE-1);
      }

      if(sdata->statistically_whitelisted == 1){
         snprintf(tmpbuf, SMALLBUFSIZE-1, "%sstatistically whitelisted\r\n", cfg->clapf_header_field);
         strncat(sdata->spaminessbuf, tmpbuf, MAXBUFSIZE-1);
      }

   } /* end of running spam check */

   return OK;
}


void getUserFromEmailAddress(struct session_data *sdata, struct __data *data, char *email, struct __config *cfg){
#ifdef HAVE_USERS
   struct timezone tz;
   struct timeval tv1, tv2;

   gettimeofday(&tv1, &tz);
 #ifdef HAVE_MEMCACHED
   if(getUserdataFromMemcached(sdata, data, email, cfg) == 0){
 #endif
      getUserdataFromEmail(sdata, email, cfg);
 #ifdef HAVE_MEMCACHED
      putUserdataToMemcached(sdata, data, email, cfg);
   }
 #endif
   gettimeofday(&tv2, &tz);
   sdata->__user += tvdiff(tv2, tv1);
#endif
}


void getPolicySettings(struct session_data *sdata, struct __data *data, struct __config *cfg, struct __config *my_cfg){
#ifdef HAVE_POLICY
   struct timezone tz;
   struct timeval tv1, tv2;

   gettimeofday(&tv1, &tz);
#ifdef HAVE_MEMCACHED
   if(getPolicyFromMemcached(sdata, data, cfg, my_cfg) == 0){
#endif
      getPolicy(sdata, cfg, my_cfg);
#ifdef HAVE_MEMCACHED
      putPolicyToMemcached(sdata, data, my_cfg);
   }
#endif
   gettimeofday(&tv2, &tz);
   sdata->__policy = tvdiff(tv2, tv1);
#endif
}


void getUsersWBL(struct session_data *sdata, struct __data *data, struct __config *cfg){
#ifdef HAVE_USERS
   struct timezone tz;
   struct timeval tv1, tv2;

   gettimeofday(&tv1, &tz);
#ifdef HAVE_MEMCACHED
   //if(getWBLFromMemcached(sdata, data, cfg) == 0){
#endif
      getWBLData(sdata, cfg);
#ifdef HAVE_MEMCACHED
      /*
       * we have to get rid of the '\r' character, because
       * it inteferes with the memcached control sequences.
       */

      /*replaceCharacterInBuffer(sdata->whitelist, '\r', 0);
      replaceCharacterInBuffer(sdata->blacklist, '\r', 0);

      //putWBLToMemcached(sdata, data, cfg);
   }*/
#endif
   gettimeofday(&tv2, &tz);
   sdata->__user += tvdiff(tv2, tv1);
#endif
}


void checkZombieSender(struct session_data *sdata, struct __data *data, struct _state *state, struct __config *cfg){
#ifdef HAVE_TRE
   int i=0;
   size_t nmatch=0;

   while(i < data->n_regex && sdata->tre != '+'){
      if(regexec(&(data->pregs[i]), state->hostname, nmatch, NULL, 0) == 0) sdata->tre = '+';
      i++;
   }

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: zombie check: %c [%d] %s", sdata->ttmpfile, sdata->tre, i, state->hostname);
#endif
}

