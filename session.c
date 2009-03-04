/*
 * session.c, 2009.03.04, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include "mime.h"
#include "av.h"
#include <clapf.h>


#ifdef NEED_MYSQL
   #include <mysql.h>
#endif

#ifdef NEED_SQLITE3
   #include <sqlite3.h>
#endif

#ifdef NEED_LDAP
   #include <ldap.h>
#endif

#ifdef HAVE_MYDB
   #include "mydb.h"
#endif


/*
 * kill child if it works too long or is frozen
 */

void kill_child(){
   syslog(LOG_PRIORITY, "child is killed by force");
   exit(0);
}


/*
 * init SMTP session
 */

void init_session_data(struct session_data *sdata){
   int i;


   sdata->fd = -1;

   memset(sdata->ttmpfile, 0, SMALLBUFSIZE);
   make_rnd_string(&(sdata->ttmpfile[0]));
   unlink(sdata->ttmpfile);

   memset(sdata->mailfrom, 0, SMALLBUFSIZE);
   memset(sdata->client_addr, 0, IPLEN);

   sdata->uid = 0;
   sdata->tot_len = 0;
   sdata->skip_id_check = 0;
   sdata->num_of_rcpt_to = 0;
   sdata->unknown_client = 0;
   sdata->blackhole = 0;
   sdata->need_signo_check = 0;

   for(i=0; i<MAX_RCPT_TO; i++) memset(sdata->rcptto[i], 0, SMALLBUFSIZE);

}


#ifdef HAVE_LIBCLAMAV
   void postfix_to_clapf(int new_sd, struct url *blackhole, struct cl_limits limits, struct cl_node *root, struct __config *cfg){
#else
   void postfix_to_clapf(int new_sd, struct url *blackhole, struct __config *cfg){
#endif

   int i, pos, n, rav=AVIR_OK, inj=ERR_REJECT, state, prevlen=0;
   char *p, *q, buf[MAXBUFSIZE], puf[MAXBUFSIZE], resp[MAXBUFSIZE], prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1], acceptbuf[MAXBUFSIZE];
   char email[SMALLBUFSIZE], email2[SMALLBUFSIZE];
   float spaminess;
   struct session_data sdata;
   struct __config my_cfg;
   int is_spam, db_conn=0;
   int rc;
   struct url *a;

   #ifdef HAVE_ANTISPAM
      char spaminessbuf[MAXBUFSIZE], reason[SMALLBUFSIZE], trainbuf[SMALLBUFSIZE], whitelistbuf[SMALLBUFSIZE];
      struct _state sstate;
      int train_mode=T_TOE, utokens;
      spaminess=DEFAULT_SPAMICITY;
      struct timezone tz;
      struct timeval tv_spam_start, tv_spam_stop, tv1, tv2;
   #endif


#ifdef HAVE_LIBCLAMAV
   /* http://www.clamav.net/doc/latest/html/node47.html */
   srand(getpid());
#endif

   alarm(cfg->session_timeout);
   signal(SIGALRM, kill_child);

   state = SMTP_STATE_INIT;

   init_session_data(&sdata);

   sdata.Nham = 0;
   sdata.Nspam = 0;

#ifdef HAVE_MYDB
   init_mydb(cfg->mydbfile, &sdata);
#endif


   /* open database connection */

   db_conn = 0;

#ifdef NEED_MYSQL
   rc = 1;
   mysql_init(&(sdata.mysql));
   mysql_options(&(sdata.mysql), MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg->mysql_connect_timeout);
   mysql_options(&(sdata.mysql), MYSQL_OPT_RECONNECT, (const char*)&rc);

   if(mysql_real_connect(&(sdata.mysql), cfg->mysqlhost, cfg->mysqluser, cfg->mysqlpwd, cfg->mysqldb, cfg->mysqlport, cfg->mysqlsocket, 0))
      db_conn = 1;
   else
      syslog(LOG_PRIORITY, "%s", ERR_MYSQL_CONNECT);
#endif

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: fork()", sdata.ttmpfile);

   /* send 220 SMTP/LMTP banner */

#ifdef HAVE_LMTP
   snprintf(buf, MAXBUFSIZE-1, LMTP_RESP_220_BANNER, cfg->hostid);
#else
   snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_220_BANNER, cfg->hostid);
#endif

   send(new_sd, buf, strlen(buf), 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

   while((n = recvtimeout(new_sd, puf, MAXBUFSIZE, 0)) > 0){
         pos = 0;

         /* accept mail data */

         if(state == SMTP_STATE_DATA){

            /* join the last 2 buffer */

            memset(last2buf, 0, 2*MAXBUFSIZE+1);
            memcpy(last2buf, prevbuf, MAXBUFSIZE);
            memcpy(last2buf+prevlen, puf, MAXBUFSIZE);

            pos = search_in_buf(last2buf, 2*MAXBUFSIZE+1, SMTP_CMD_PERIOD, 5);
            if(pos > 0){

	       /* fix position */
	       pos = pos - prevlen + strlen(SMTP_CMD_PERIOD);

               /* write data only to (and including) the trailing period (.) */
               write(sdata.fd, puf, pos);
               sdata.tot_len += pos;


               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: (.)", sdata.ttmpfile);

               state = SMTP_STATE_PERIOD;

               /* make sure we had a successful read, 2007.11.05, SJ */

               rc = fsync(sdata.fd);
               close(sdata.fd);

               if(rc){
                  syslog(LOG_PRIORITY, "failed writing data: %s", sdata.ttmpfile);

               #ifdef HAVE_LMTP
                  for(i=0; i<sdata.num_of_rcpt_to; i++){
               #endif

                     send(new_sd, SMTP_RESP_421_ERR_WRITE_FAILED, strlen(SMTP_RESP_421_ERR_WRITE_FAILED), 0);

               #ifdef HAVE_LMTP
                  }
               #endif

                  memset(puf, 0, MAXBUFSIZE);
                  goto AFTER_PERIOD;
               }

               //write_delivery_info(&sdata, cfg->workdir);

               /* parse message */
               sstate = parse_message(sdata.ttmpfile, &sdata, cfg);

               if(sstate.has_base64 == 0 && cfg->always_scan_message == 0) sdata.need_scan = 0;
               else sdata.need_scan = 1;


               /* do antivirus check, if we have to */

            #ifdef HAVE_ANTIVIRUS
               #ifdef HAVE_LIBCLAMAV
                  rav = do_av_check(&sdata, email, email2, limits, root, cfg);
               #else
                  rav = do_av_check(&sdata, email, email2, cfg);
               #endif
            #endif


               /* open database backend handler */

            #ifdef NEED_SQLITE3
               db_conn = 0;
               rc = sqlite3_open(cfg->sqlite3, &(sdata.db));
               if(rc)
                  syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_SQLITE3_OPEN);
               else {
                  db_conn = 1;
                  rc = sqlite3_exec(sdata.db, cfg->sqlite3_pragma, 0, 0, NULL);
                  if(rc != SQLITE_OK) syslog(LOG_PRIORITY, "%s: could not set pragma", sdata.ttmpfile);
               }
            #endif
            #ifdef NEED_LDAP
               sdata.ldap = do_bind_ldap(cfg->ldap_host, cfg->ldap_user, cfg->ldap_pwd, cfg->ldap_use_tls);
            #endif


               /* copy default config from clapf.conf, to enable policy support */

               memcpy(&my_cfg, cfg, sizeof(struct __config));

            #ifdef HAVE_LMTP
               for(i=0; i<sdata.num_of_rcpt_to; i++){
            #else
               i = 0;
            #endif
                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: round %d in injection", sdata.ttmpfile, i);

                  memset(acceptbuf, 0, MAXBUFSIZE);
                  memset(email, 0, SMALLBUFSIZE);

                  is_spam = 0;
                  spaminess = DEFAULT_SPAMICITY;

                  extract_email(sdata.rcptto[i], email);

                  /* get user from 'RCPT TO:', 2008.11.24, SJ */

               #ifdef HAVE_USERS
                  get_user_from_email(&sdata, email, &my_cfg);
               #endif

                  /* read policy, 2008.11.24, SJ */

               #ifdef HAVE_POLICY
                  if(sdata.policy_group > 0) get_policy(&sdata, cfg, &my_cfg);
               #endif


                  if(rav == AVIR_VIRUS){
                     if(my_cfg.deliver_infected_email == 1) goto END_OF_SPAM_CHECK;

                     snprintf(acceptbuf, MAXBUFSIZE-1, "%s <%s>\r\n", SMTP_RESP_550_ERR_PREF, email);

                     if(my_cfg.silently_discard_infected_email == 1)
                        snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                     else
                        snprintf(acceptbuf, MAXBUFSIZE-1, "550 %s %s\r\n", sdata.ttmpfile, email);

                     goto SEND_RESULT;
                  }

               #ifdef HAVE_ANTISPAM
                  memset(reason, 0, SMALLBUFSIZE);
                  memset(trainbuf, 0, SMALLBUFSIZE);
                  memset(whitelistbuf, 0, SMALLBUFSIZE);
                  memset(spaminessbuf, 0, MAXBUFSIZE);

                  gettimeofday(&tv_spam_start, &tz);

                  /* is it a training request? */

                  if(sdata.num_of_rcpt_to == 1 && (strcasestr(sdata.rcptto[0], "+spam@") || strcasestr(sdata.rcptto[0], "+ham@") || strncmp(email, "spam@", 5) == 0 || strncmp(email, "ham@", 4) == 0 ) ){
                  #ifdef HAVE_USERS
                     /* get user from 'MAIL FROM:', 2008.10.25, SJ */

                     get_user_from_email(&sdata, email2, cfg);

                     /*
                        If not found, then try to get it from the RCPT TO address.

                        This may happen if your email address is xy@mail.domain.com,
                        but xy@domain.com is set in your email client application as
                        your email address.
                        In this case send training emails to xy+spam@mail.domain.com

                      */
                     if(sdata.name[0] == 0) get_user_from_email(&sdata, email, cfg);

                     do_training(&sdata, email, &acceptbuf[0], &my_cfg);
                     goto SEND_RESULT;

                  #else

                     /* if you have not specified --with-userdb=.... then we don't know
                        much about the recipient, so you have to do the training with
                        spamdrop
                      */

                     snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%s\r\n%sTraining request\r\n", cfg->clapf_header_field, sdata.ttmpfile, cfg->clapf_header_field);
                     goto END_OF_SPAM_CHECK;
                  #endif
                  }


                  /* force spamcheck if the message sent to the blackhole */
                  if(sdata.blackhole == 1) my_cfg.use_antispam = 1;

                  /* run statistical antispam check */


                  if(my_cfg.use_antispam == 1 && (my_cfg.max_message_size_to_filter == 0 || sdata.tot_len < my_cfg.max_message_size_to_filter) ){

                  #ifdef SPAMC_EMUL
                     rc = spamc_emul(sdata.ttmpfile, sdata.tot_len, cfg);
                     gettimeofday(&tv_spam_stop, &tz);

                     if(rc == 1){
                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%s\r\n%s%ld ms\r\n%s", // !!
                              cfg->clapf_header_field, sdata.ttmpfile, cfg->clapf_header_field, tvdiff(tv_spam_stop, tv_spam_start)/1000, cfg->clapf_spam_header_field);

                        spaminess = 0.99;

                        syslog(LOG_PRIORITY, "%s: SPAM", sdata.ttmpfile);
                     }
                     else {
                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%s\r\n%s%ld ms\r\n",
                              cfg->clapf_header_field, sdata.ttmpfile, cfg->clapf_header_field, tvdiff(tv_spam_stop, tv_spam_start)/1000);
                     }

                     syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, spaminess, sdata.tot_len, tvdiff(tv_spam_stop, tv_spam_start)/1000);

                     goto END_OF_SPAM_CHECK;
                  #endif

                  #ifdef HAVE_MYDB
                     spaminess = bayes_file(&sdata, &sstate, cfg);
                     goto END_OF_TRAINING;
                  #endif

                     /* some MTAs strip our signo from the bounce. So if we would raise the spaminess
                      * then we may commit a false positive. Thus in case of a missing signo, let
                      * the statistical analysis decide the fate of a dummy bounce message. 2009.01.20, SJ
                      */

                     if(sdata.need_signo_check == 1){
                        if(sstate.found_our_signo == 1){
                           syslog(LOG_PRIORITY, "%s: bounce message, found our signo", sdata.ttmpfile);
                           goto END_OF_TRAINING;
                        }
                        else
                           syslog(LOG_PRIORITY, "%s: looks like a bounce, but our signo is missing", sdata.ttmpfile);
                     }


                     if(db_conn == 1){

                        /* check whitelist first */

                     #ifdef HAVE_WHITELIST
                        if(is_sender_on_black_or_white_list(&sdata, email2, SQL_WHITE_LIST, &my_cfg) == 1){
                           syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata.ttmpfile, email2);
                           snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on whitelist\r\n", cfg->clapf_header_field);
                           goto END_OF_TRAINING;
                        }
                     #endif

                        /* then give blacklist a try */

                     #ifdef HAVE_BLACKLIST
                        if(is_sender_on_black_or_white_list(&sdata, email2, SQL_BLACK_LIST, &my_cfg) == 1){
                           syslog(LOG_PRIORITY, "%s: sender (%s) found on blacklist", sdata.ttmpfile, email2);
                           snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on blacklist\r\n", cfg->clapf_header_field);

                           snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                           goto SEND_RESULT;
                        }
                     #endif

                        if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running Bayesian test", sdata.ttmpfile);
                        spaminess = bayes_file(&sdata, &sstate, &my_cfg);

                        if(spaminess > 0.9999) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg->clapf_header_field, MSG_ABSOLUTELY_SPAM);

                        /* skip TUM training on a blackhole message, unless it may learn a missed spam as a good email */

                        if(
                           (sdata.blackhole == 0 && my_cfg.training_mode == T_TUM && ( (spaminess >= my_cfg.spam_overall_limit && spaminess < 0.99) || (spaminess < my_cfg.max_ham_spamicity && spaminess > 0.1) )) ||
                           (my_cfg.initial_1000_learning == 1 && (sdata.Nham < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED || sdata.Nspam < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED))
                        )
                        {

                           if(spaminess >= my_cfg.spam_overall_limit){
                              is_spam = 1;
                              syslog(LOG_PRIORITY, "%s: TUM training a spam", sdata.ttmpfile);
                           }
                           else {
                              is_spam = 0;
                              syslog(LOG_PRIORITY, "%s: TUM training a ham", sdata.ttmpfile);
                           }

                           snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg->clapf_header_field);

                           train_message(&sdata, &sstate, 1, is_spam, train_mode, &my_cfg);
                        }


                        /* training a blackhole message as spam, if we have to */

                        if(sdata.blackhole == 1){
                           snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg->clapf_header_field, MSG_BLACKHOLED);

                           if(spaminess < 0.99){
                              syslog(LOG_PRIORITY, "%s: training on a blackhole message", sdata.ttmpfile);
                              snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM on blackhole\r\n", cfg->clapf_header_field);
                              train_message(&sdata, &sstate, MAX_ITERATIVE_TRAIN_LOOPS, 1, T_TOE, &my_cfg);
                           }
                        }

                     } 


                     /* update token timestamps */

                     if(cfg->update_tokens == 1){
                        gettimeofday(&tv1, &tz);
                     #ifdef HAVE_MYSQL
                        utokens = update_mysql_tokens(sdata.mysql, sstate.token_hash, sdata.uid);
                     #endif
                     #ifdef HAVE_SQLITE3
                        utokens = update_sqlite3_tokens(sdata.db, sstate.token_hash);
                     #endif
                        gettimeofday(&tv2, &tz);
                        if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: updated %d/%ld tokens: %ld [ms]", sdata.ttmpfile, utokens, sstate.n_token, tvdiff(tv2, tv1)/1000);
                     } 

                  END_OF_TRAINING:

                     /* save email to queue */

                  #ifdef HAVE_STORE
                     if(sdata.uid > 0) save_email_to_queue(&sdata, spaminess, &my_cfg);
                  #endif

                     gettimeofday(&tv_spam_stop, &tz);
                     syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, spaminess, sdata.tot_len, tvdiff(tv_spam_stop, tv_spam_start)/1000);

                     if(spaminess >= cfg->spam_overall_limit){
                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s%ld ms\r\n%s%s%s%s", // !!
                              cfg->clapf_header_field, spaminess, cfg->clapf_header_field, sdata.ttmpfile, cfg->clapf_header_field,
                                    tvdiff(tv_spam_stop, tv_spam_start)/1000, reason, trainbuf, whitelistbuf, cfg->clapf_spam_header_field);

                        log_ham_spam_per_email(sdata.ttmpfile, email, 1);
                     }
                     else {
                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s%ld ms\r\n%s%s%s",
                               cfg->clapf_header_field, spaminess, cfg->clapf_header_field, sdata.ttmpfile, cfg->clapf_header_field, tvdiff(tv_spam_stop, tv_spam_start)/1000, reason, trainbuf, whitelistbuf);

                        log_ham_spam_per_email(sdata.ttmpfile, email, 0);
                     }


                  } /* end of running spam check */

                  else {
                     /* set a reasonable stuff if we skipped the antispam check */
                     snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%s\r\n", cfg->clapf_header_field, sdata.ttmpfile);
                  }

               END_OF_SPAM_CHECK:

                  /* then inject message back */

                  if(spaminess >= my_cfg.spam_overall_limit){

                    /* shall we redirect the message into oblivion? 2007.02.07, SJ */
                    if(spaminess >= my_cfg.spaminess_oblivion_limit)
                       inj = ERR_DROP_SPAM;
                    else
                       inj = inject_mail(&sdata, i, cfg->spam_smtp_addr, cfg->spam_smtp_port, spaminessbuf, &my_cfg, NULL);
                  }
                  else {
                     inj = inject_mail(&sdata, i, cfg->postfix_addr, cfg->postfix_port, spaminessbuf, &my_cfg, NULL);
                  }

               #else
               END_OF_SPAM_CHECK:
                  inj = inject_mail(&sdata, i, cfg->postfix_addr, cfg->postfix_port, NULL, &my_cfg, NULL);
               #endif

                  /* set the accept buffer */

                  if(inj == OK || inj == ERR_DROP_SPAM){
                     snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                  }
                  else if(inj == ERR_REJECT){
                     snprintf(acceptbuf, MAXBUFSIZE-1, "550 %s <%s>\r\n", sdata.ttmpfile, email);
                  }
                  else {
                     snprintf(acceptbuf, MAXBUFSIZE-1, "451 %s <%s>\r\n", sdata.ttmpfile, email);
                  }

               SEND_RESULT:
                  send(new_sd, acceptbuf, strlen(acceptbuf), 0);

                  if(inj == ERR_DROP_SPAM) syslog(LOG_PRIORITY, "%s: dropped spam", sdata.ttmpfile);
                  else if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, acceptbuf);

            #ifdef HAVE_LMTP
               } /* for */
            #endif


               unlink(sdata.ttmpfile);
               //unlink(sdata.deliveryinfo);


               /* free state lists */

               free_list(sstate.urls);
               clearhash(sstate.token_hash, 0);


               /* close database backend handler */

            #ifdef NEED_SQLITE3
               db_conn = 0;
               sqlite3_close(sdata.db);
               rc = SQLITE_ERROR;
            #endif
            #ifdef NEED_IN_LDAP
               ldap_unbind_s(sdata.ldap);
            #endif

               /* if we have nothing after the trailing (.), we can read
                  the next command from the network */

               if(puf[n-3] == '.' && puf[n-2] == '\r' && puf[n-1] == '\n') continue;


               /* if we left something in the puffer, we are ready to proceed
                  to handle the additional commands */

            } /* PERIOD found */
            else {
               write(sdata.fd, puf, n);
               sdata.tot_len += n;

               memcpy(prevbuf, puf, n);
               prevlen = n;

               continue;
            }

         } /* SMTP DATA */

AFTER_PERIOD:

      /* handle smtp commands */

      memset(resp, 0, MAXBUFSIZE);

      p = &puf[pos];

      while((p = split_str(p, "\r\n", buf, MAXBUFSIZE-1))){
         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

         // HELO/EHLO

         if(strncasecmp(buf, SMTP_CMD_HELO, strlen(SMTP_CMD_HELO)) == 0 || strncasecmp(buf, SMTP_CMD_EHLO, strlen(SMTP_CMD_EHLO)) == 0 || strncasecmp(buf, LMTP_CMD_LHLO, strlen(LMTP_CMD_LHLO)) == 0){
            if(state == SMTP_STATE_INIT) state = SMTP_STATE_HELO;

            snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_250_EXTENSIONS, cfg->hostid);
            strncat(resp, buf, MAXBUFSIZE-1);

            continue;

            /* FIXME: implement the ENHANCEDSTATUSCODE extensions */
         }

         // XFORWARD

         if(strncasecmp(buf, SMTP_CMD_XFORWARD, strlen(SMTP_CMD_XFORWARD)) == 0){

            /* extract client address */

            trim(buf);
            q = strstr(buf, "ADDR=");
            if(q){
               snprintf(sdata.client_addr, IPLEN-1, q+5);
               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: client address: %s", sdata.ttmpfile, sdata.client_addr);
            }

            /*
             * other possible XFORWARD variables
             *
             * XFORWARD NAME=83-131-14-231.adsl.net.t-com.hr ADDR=83.131.14.231..
             * XFORWARD PROTO=SMTP HELO=rhwfsvji..
             */

            /* note if the client is unknown, 2007.12.06, SJ */

            if(strstr(buf, " NAME=unknown ")){
               sdata.unknown_client = 1;
            }

            strncat(resp, SMTP_RESP_250_OK, MAXBUFSIZE-1);

            continue;
         }

         // MAIL FROM

         if(strncasecmp(buf, SMTP_CMD_MAIL_FROM, strlen(SMTP_CMD_MAIL_FROM)) == 0){

            if(state != SMTP_STATE_HELO){
               strncat(resp, SMTP_RESP_503_ERR, MAXBUFSIZE-1);
            } 
            else {
               state = SMTP_STATE_MAIL_FROM;

               /* if we ever need the SIZE argumentum from the MAIL FROM command */

               /*q = strstr(buf, " SIZE=");
               if(q){
                  *q = '\0';
                  i = strcspn(q+6, " \r\n");
                  if(i > 0){
                     *(q+6+i) = '\0';
                     if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: size of message: *%s*", sdata.ttmpfile, q+6);
                  }
               }*/

               snprintf(sdata.mailfrom, SMALLBUFSIZE-1, "%s\r\n", buf);


               memset(email2, 0, SMALLBUFSIZE);
               extract_email(sdata.mailfrom, email2);

               //snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_250_210_OK, email2);
               strncat(resp, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK));

               if((strstr(sdata.mailfrom, "MAILER-DAEMON") || strstr(sdata.mailfrom, "<>")) && strlen(cfg->our_signo) > 3) sdata.need_signo_check = 1;
            }

            continue;
         }

         // RCPT TO

         if(strncasecmp(buf, SMTP_CMD_RCPT_TO, strlen(SMTP_CMD_RCPT_TO)) == 0){

            if(state == SMTP_STATE_MAIL_FROM || state == SMTP_STATE_RCPT_TO){
               if(sdata.num_of_rcpt_to < MAX_RCPT_TO){
                  //memcpy(sdata.rcptto[sdata.num_of_rcpt_to], buf, SMALLBUFSIZE-1);
                  snprintf(sdata.rcptto[sdata.num_of_rcpt_to], SMALLBUFSIZE-1, "%s\r\n", buf);
                  sdata.num_of_rcpt_to++;
               }

               state = SMTP_STATE_RCPT_TO;

               /* check against blackhole addresses */

               extract_email(buf, email);

               if(blackhole){
                  a = blackhole;

                  while(a){
                     if(strcmp(a->url_str, email) == 0){
                        syslog(LOG_PRIORITY, "we have %s on the blacklist", email);
                        sdata.blackhole = 1;
                        break;
                     }
                     a = a->r;
                  }
               }

               /* check against DHA trap address list, 2007.11.06, SJ */

            #ifdef HAVE_BLACKHOLE
               if(strlen(cfg->dha_trap_address_list) > 4){
                  if(strstr(cfg->dha_trap_address_list, email)){
                     syslog(LOG_PRIORITY, "%s: %s trapped with %s on my DHA list", sdata.ttmpfile, sdata.client_addr, email);
                     put_ip_to_dir(cfg->blackhole_path, sdata.client_addr);
                  }
               }
            #endif

               strncat(resp, SMTP_RESP_250_OK, MAXBUFSIZE-1);
            }
            else {
               strncat(resp, SMTP_RESP_503_ERR, MAXBUFSIZE-1);
            }

            continue;
         }

         // DATA

         if(strncasecmp(buf, SMTP_CMD_DATA, strlen(SMTP_CMD_DATA)) == 0){

            memset(last2buf, 0, 2*MAXBUFSIZE+1);
            memset(prevbuf, 0, MAXBUFSIZE);
            inj = ERR_REJECT;
            prevlen = 0;

            if(state != SMTP_STATE_RCPT_TO){
               strncat(resp, SMTP_RESP_503_ERR, MAXBUFSIZE-1);
            }
            else {
               sdata.fd = open(sdata.ttmpfile, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
               if(sdata.fd == -1){
                  syslog(LOG_PRIORITY, "%s: %s", ERR_OPEN_TMP_FILE, sdata.ttmpfile);
                  strncat(resp, SMTP_RESP_451_ERR, MAXBUFSIZE-1);
               }
               else {
                  state = SMTP_STATE_DATA;
                  strncat(resp, SMTP_RESP_354_DATA_OK, MAXBUFSIZE-1);
               }

            }

            continue; 
         }

         // QUIT

         if(strncasecmp(buf, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT)) == 0){

            state = SMTP_STATE_FINISHED;

            snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_221_GOODBYE, cfg->hostid);
            send(new_sd, buf, strlen(buf), 0);
            if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

            unlink(sdata.ttmpfile);
            if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);

            goto QUITTING;
         }

         // NOOP

         if(strncasecmp(buf, SMTP_CMD_NOOP, strlen(SMTP_CMD_NOOP)) == 0){
            strncat(resp, SMTP_RESP_250_OK, MAXBUFSIZE-1);
            continue;
         }


         // RSET

         if(strncasecmp(buf, SMTP_CMD_RESET, strlen(SMTP_CMD_RESET)) == 0){

            /* we must send a 250 Ok */

            strncat(resp, SMTP_RESP_250_OK, MAXBUFSIZE-1);

            if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);
            unlink(sdata.ttmpfile);

            init_session_data(&sdata);

            state = SMTP_STATE_HELO;

            continue;
         }

         /* by default send 502 command not implemented message */
         //strncat(resp, SMTP_RESP_502_ERR, MAXBUFSIZE-1);
         strncat(resp, SMTP_RESP_450_ERR_CMD_NOT_IMPLEMENTED, MAXBUFSIZE-1);
      }


      /* now we can send our buffered response */

      if(strlen(resp) > 0){
         send(new_sd, resp, strlen(resp), 0);
         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, resp);
         memset(resp, 0, MAXBUFSIZE);
      }

   } /* while */

   /*
    * if we are not in SMTP_STATE_QUIT and the message was not injected,
    * ie. we have timed out than send back 421 error message, 2005.12.29, SJ
    */

   if(state < SMTP_STATE_QUIT && inj != OK){
      snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_421_ERR, cfg->hostid);
      send(new_sd, buf, strlen(buf), 0);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

      goto QUITTING;
   }


QUITTING:

#ifdef HAVE_MYDB
   close_mydb(sdata.mhash);
#endif
#ifdef NEED_MYSQL
   mysql_close(&(sdata.mysql));
#endif

   if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "child has finished");

   _exit(0);
}
