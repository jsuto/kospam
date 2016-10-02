/*
 * session.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <clapf.h>


int handle_smtp_session(int new_sd, struct __data *data, struct __config *cfg){
   int i, k, ret, pos, n, inj=ERR, smtp_state, prevlen=0;
   char *p, *q, buf[MAXBUFSIZE], puf[MAXBUFSIZE], resp[MAXBUFSIZE], inject_resp[MAXBUFSIZE], prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1];
   char virusinfo[SMALLBUFSIZE], delay[SMALLBUFSIZE], tmpbuf[SMALLBUFSIZE], recipient[SMALLBUFSIZE];
   struct session_data sdata;
   struct __state state;
   struct __config my_cfg;
   struct __counters counters;
   int db_conn=0;
   int rc;
   struct timezone tz;
   struct timeval tv1, tv2;
   int starttls = 0;
   char ssl_error[SMALLBUFSIZE];


   struct request_info req;

   request_init(&req, RQ_DAEMON, PROGNAME, RQ_FILE, new_sd, 0);
   fromhost(&req);
   if(!hosts_access(&req)){
      send(new_sd, SMTP_RESP_550_ERR_YOU_ARE_BANNED_BY_LOCAL_POLICY, strlen(SMTP_RESP_550_ERR_YOU_ARE_BANNED_BY_LOCAL_POLICY), 0);
      syslog(LOG_PRIORITY, "denied connection from %s by tcp_wrappers", eval_client(&req));
      return 0;
   }

   srand(getpid());

   smtp_state = SMTP_STATE_INIT;

   init_session_data(&sdata, cfg);
   sdata.tls = 0;

   bzero(&counters, sizeof(counters));


   /* open database connection */

   db_conn = 0;

#ifdef NEED_MYSQL
   if(open_database(&sdata, cfg) == OK){
      db_conn = 1;
   }
   else
      syslog(LOG_PRIORITY, "error: %s", ERR_MYSQL_CONNECT);
#endif

   if(db_conn == 0){
      snprintf(buf, sizeof(buf)-1, SMTP_RESP_421_ERR_TMP, cfg->hostid);
      send(new_sd, buf, strlen(buf), 0);
      return 0;
   }


   gettimeofday(&tv1, &tz);

   if(cfg->server_mode == SMTP_MODE)
      snprintf(buf, sizeof(buf)-1, SMTP_RESP_220_BANNER, cfg->hostid);
   else
      snprintf(buf, sizeof(buf)-1, LMTP_RESP_220_BANNER, cfg->hostid);


   update_child_stat_entry(&sdata, 'R', 0);

   send(new_sd, buf, strlen(buf), 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

   while((n = recvtimeoutssl(new_sd, puf, sizeof(puf), TIMEOUT, sdata.tls, data->ssl)) > 0){
         pos = 0;

         /* accept mail data */

         if(smtp_state == SMTP_STATE_DATA){

            /* join the last 2 buffer */

            memset(last2buf, 0, sizeof(last2buf));
            memcpy(last2buf, prevbuf, sizeof(prevbuf));
            memcpy(last2buf+prevlen, puf, sizeof(puf));


            pos = search_string_in_buffer(last2buf, sizeof(last2buf), SMTP_CMD_PERIOD, 5);
            if(pos > 0){

	       /* fix position */
               pos = pos - prevlen;

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: period found", sdata.ttmpfile);


               /* write data only to (and including) the trailing period (.) */
               ret = write(sdata.fd, puf, pos);
               sdata.tot_len += ret;

               /* fix position! */
               pos += strlen(SMTP_CMD_PERIOD);

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: (.)", sdata.ttmpfile);


               smtp_state = SMTP_STATE_PERIOD;

               /* make sure we had a successful read */

               rc = fsync(sdata.fd);
               close(sdata.fd);

               gettimeofday(&tv2, &tz);
               sdata.__acquire = tvdiff(tv2, tv1);

               if(rc){
                  syslog(LOG_PRIORITY, "error: failed writing data: %s", sdata.ttmpfile);

                  if(cfg->server_mode == SMTP_MODE) k = 1; else k = sdata.num_of_rcpt_to;

                  for(i=0; i<k; i++){
                     write1(new_sd, SMTP_RESP_421_ERR_WRITE_FAILED, strlen(SMTP_RESP_421_ERR_WRITE_FAILED), sdata.tls, data->ssl);
                  } 

                  memset(puf, 0, sizeof(puf));
                  goto AFTER_PERIOD;
               }


               gettimeofday(&tv1, &tz);
               update_child_stat_entry(&sdata, 'P', 0);
               state = parse_message(&sdata, 1, data, cfg);
               post_parse(&sdata, &state, cfg);
               gettimeofday(&tv2, &tz);
               sdata.__parsed = tvdiff(tv2, tv1);

               sdata.rav = check_for_known_bad_attachments(&sdata, &state);
               if(sdata.rav == AVIR_VIRUS) snprintf(virusinfo, sizeof(virusinfo)-1, "MARKED.AS.MALWARE");

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: parsed message", sdata.ttmpfile);

               if(state.n_attachments > 0 || cfg->always_scan_message == 1) sdata.need_scan = 1;
               else sdata.need_scan = 0;

               if(is_item_on_list(sdata.ip, cfg->mynetwork, "") == 1){
                  syslog(LOG_PRIORITY, "%s: client ip (%s) on mynetwork", sdata.ttmpfile, sdata.ip);
                  sdata.mynetwork = 1;
               }


            #ifdef HAVE_ANTIVIRUS
               if(cfg->use_antivirus == 1 && sdata.rav == AVIR_OK){
                  gettimeofday(&tv1, &tz);
                  update_child_stat_entry(&sdata, 'A', 0);
                  sdata.rav = do_av_check(&sdata, &virusinfo[0], data, cfg);
                  gettimeofday(&tv2, &tz);
                  sdata.__av = tvdiff(tv2, tv1);
               }
            #endif


               snprintf(sdata.acceptbuf, SMALLBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, sdata.rcptto[i]);

               if(cfg->server_mode == SMTP_MODE) k = 1; else k = sdata.num_of_rcpt_to;

               for(i=0; i<k; i++){
                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: round %d in injection", sdata.ttmpfile, i);

                  inj = ERR;
                  snprintf(inject_resp, sizeof(inject_resp)-1, "undef");


                  /* copy default config from clapf.conf, to enable policy support */
                  memcpy(&my_cfg, cfg, sizeof(struct __config));


                  if(db_conn == 1){

                     update_child_stat_entry(&sdata, 'S', 0);

                     snprintf(recipient, sizeof(recipient)-1, "%s", sdata.rcptto[i]);
                     extract_verp_address(recipient);

                     if(check_spam(&sdata, &state, data, sdata.fromemail, recipient, cfg, &my_cfg) == DISCARD){
                        snprintf(inject_resp, sizeof(inject_resp)-1, "discarded");
                        goto SEND_RESULT;
                     }

                  }


                  /* inject message back */

                  gettimeofday(&tv1, &tz);

                  update_child_stat_entry(&sdata, 'W', 0);

                  if( (sdata.rav == AVIR_VIRUS && cfg->silently_discard_infected_email == 1) || sdata.spaminess >= my_cfg.spaminess_oblivion_limit ){
                     inj = OK;
                     snprintf(inject_resp, sizeof(inject_resp)-1, "discarded");
                  }
                  else if(sdata.spaminess >= my_cfg.spam_overall_limit){
                     strncat(sdata.spaminessbuf, cfg->clapf_spam_header_field, MAXBUFSIZE-1);
                     inj = inject_mail(&sdata, i, sdata.spaminessbuf, &inject_resp[0], &my_cfg);
                  }
                  else {
                     inj = inject_mail(&sdata, i, sdata.spaminessbuf, &inject_resp[0], &my_cfg);
                  }

                  gettimeofday(&tv2, &tz);
                  sdata.__inject = tvdiff(tv2, tv1);



                  /* set the accept buffer to send back to postfix */

                  switch(inj) {

                     case OK:
                                 snprintf(sdata.acceptbuf, SMALLBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, sdata.rcptto[i]);
                                 break;

                     case ERR_REJECT:
                                 snprintf(sdata.acceptbuf, SMALLBUFSIZE-1, "550 %s <%s>\r\n", sdata.ttmpfile, sdata.rcptto[i]);
                                 break;

                     default:
                                 snprintf(sdata.acceptbuf, SMALLBUFSIZE-1, "451 %s <%s>\r\n", sdata.ttmpfile, sdata.rcptto[i]);
                                 break;
                  }


               SEND_RESULT:

                  write1(new_sd, sdata.acceptbuf, strlen(sdata.acceptbuf), sdata.tls, data->ssl);

                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, sdata.acceptbuf);

                  counters.c_rcvd++;

                  snprintf(delay, sizeof(delay)-1, "delay=%.2f, delays=%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f", 
                               (sdata.__acquire+sdata.__parsed+sdata.__av+sdata.__user+sdata.__policy+sdata.__minefield+sdata.__as+sdata.__training+sdata.__update+sdata.__store+sdata.__inject)/1000000.0,
                                   sdata.__acquire/1000000.0, sdata.__parsed/1000000.0, sdata.__av/1000000.0, sdata.__user/1000000.0, sdata.__policy/1000000.0, sdata.__minefield/1000000.0,
                                       sdata.__as/1000000.0, sdata.__training/1000000.0, sdata.__update/1000000.0, sdata.__store/1000000.0, sdata.__inject/1000000.0);

                  if(sdata.rav == AVIR_VIRUS){
                     counters.c_virus++;
                     sdata.status = S_VIRUS;
                     snprintf(tmpbuf, sizeof(tmpbuf)-1, "VIRUS (%s)", virusinfo);
                  } else if(sdata.spaminess >= my_cfg.spam_overall_limit){
                     sdata.status = S_SPAM;
                     counters.c_spam++;
                     snprintf(tmpbuf, sizeof(tmpbuf)-1, "SPAM");
                  } else {
                     sdata.status = S_HAM;
                     counters.c_ham++;
                     snprintf(tmpbuf, sizeof(tmpbuf)-1, "HAM");

                     if(sdata.spaminess < my_cfg.spam_overall_limit && sdata.spaminess > my_cfg.possible_spam_limit) counters.c_possible_spam++;
                     else if(sdata.spaminess < my_cfg.possible_spam_limit && sdata.spaminess > my_cfg.max_ham_spamicity) counters.c_unsure++;
                  }

                  if(cfg->log_subject == 1) syslog(LOG_PRIORITY, "%s: subject=%s", sdata.ttmpfile, state.b_subject);
                  syslog(LOG_PRIORITY, "%s: from=%s, to=%s, result=%s/%.4f, size=%d, attachments=%d, relay=%s:%d, %s, status=%s", sdata.ttmpfile, sdata.fromemail, sdata.rcptto[i], tmpbuf, sdata.spaminess, sdata.tot_len, state.n_attachments, my_cfg.smtp_addr, my_cfg.smtp_port, delay, inject_resp);

               } /* for */


               if(sdata.training_request == 0){
                  if(write_history(&sdata, &state, inject_resp, &my_cfg) != OK) syslog(LOG_PRIORITY, "%s: error: failed inserting to history", sdata.ttmpfile);
               }

               unlink(sdata.ttmpfile);

               clearhash(state.token_hash);
               clearhash(state.url);


               /* if we have nothing after the trailing (.), we can read
                  the next command from the network */

               if(puf[n-3] == '.' && puf[n-2] == '\r' && puf[n-1] == '\n') continue;


               /* if we left something in the puffer, we are ready to proceed
                  to handle the additional commands, such as QUIT */

               /* if we miss the trailing \r\n, ie. we need another read */

               if(puf[n-2] != '\r' && puf[n-1] != '\n'){
                  memmove(puf, puf+pos, n-pos);
                  memset(puf+n-pos, 0, sizeof(puf)-n+pos);
                  i = recvtimeout(new_sd, buf, sizeof(buf), TIMEOUT);
                  strncat(puf, buf, sizeof(puf)-1-n+pos);
                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: partial read: %s", sdata.ttmpfile, puf);
                  pos = 0;
               }

            } /* PERIOD found */
            else {
               ret = write(sdata.fd, puf, n);
               sdata.tot_len += ret;

               memcpy(prevbuf, puf, n);
               prevlen = n;

               continue;
            }

         } /* SMTP DATA */

AFTER_PERIOD:

      /* handle smtp commands */

      memset(resp, 0, sizeof(resp));

      p = &puf[pos];

      while((p = split_str(p, "\r\n", buf, sizeof(buf)-1))){
         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);


         if(strncasecmp(buf, SMTP_CMD_EHLO, strlen(SMTP_CMD_EHLO)) == 0 || strncasecmp(buf, LMTP_CMD_LHLO, strlen(LMTP_CMD_LHLO)) == 0){
            if(smtp_state == SMTP_STATE_INIT) smtp_state = SMTP_STATE_HELO;

            if(sdata.tls == 0) snprintf(buf, sizeof(buf)-1, SMTP_RESP_250_EXTENSIONS, cfg->hostid, data->starttls);
            else snprintf(buf, sizeof(buf)-1, SMTP_RESP_250_EXTENSIONS, cfg->hostid, "");

            strncat(resp, buf, sizeof(resp)-1);

            continue;

            /* FIXME: implement the ENHANCEDSTATUSCODE extensions */
         }


         if(strncasecmp(buf, SMTP_CMD_HELO, strlen(SMTP_CMD_HELO)) == 0){
            if(smtp_state == SMTP_STATE_INIT) smtp_state = SMTP_STATE_HELO;

            strncat(resp, SMTP_RESP_250_OK, sizeof(resp)-1);

            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_XFORWARD, strlen(SMTP_CMD_XFORWARD)) == 0){

            /*
             * XFORWARD NAME=83-131-14-231.adsl.net.t-com.hr ADDR=83.131.14.231..
             * XFORWARD PROTO=SMTP HELO=rhwfsvji..
             */

            trim_buffer(buf);

            q = strstr(buf, "ADDR=");
            if(q){
               snprintf(sdata.ip, SMALLBUFSIZE-1, "%s", q+5);
               q = strchr(sdata.ip, ' ');
               if(q) *q = '\0';

               sdata.ipcnt = 2; // to prevent the parser to overwrite ip/hostname

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: smtp client xforward address: *%s*", sdata.ttmpfile, sdata.ip);
            }

            q = strstr(buf, "NAME=");
            if(q){
               snprintf(sdata.hostname, SMALLBUFSIZE-1, "%s", q+5);
               q = strchr(sdata.hostname, ' ');
               if(q) *q = '\0';

               sdata.ipcnt = 2; // to prevent the parser to overwrite ip/hostname

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: smtp client xforward hostname: *%s*", sdata.ttmpfile, sdata.hostname);
            }


            strncat(resp, SMTP_RESP_250_OK, sizeof(resp)-1);

            continue;
         }


         if(cfg->tls_enable > 0 && strncasecmp(buf, SMTP_CMD_STARTTLS, strlen(SMTP_CMD_STARTTLS)) == 0 && strlen(data->starttls) > 4 && sdata.tls == 0){
            if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: starttls request from client", sdata.ttmpfile);

            if(data->ctx){
               data->ssl = SSL_new(data->ctx);
               if(data->ssl){

                  SSL_set_options(data->ssl, SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);

                  if(SSL_set_fd(data->ssl, new_sd) == 1){
                     strncat(resp, SMTP_RESP_220_READY_TO_START_TLS, sizeof(resp)-1);
                     starttls = 1;
                     smtp_state = SMTP_STATE_INIT;

                     continue;
                  } syslog(LOG_PRIORITY, "%s: error: SSL_set_fd() failed", sdata.ttmpfile);
               } syslog(LOG_PRIORITY, "%s: error: SSL_new() failed", sdata.ttmpfile);
            } syslog(LOG_PRIORITY, "%s: error: SSL ctx is null!", sdata.ttmpfile);


            strncat(resp, SMTP_RESP_454_ERR_TLS_TEMP_ERROR, sizeof(resp)-1);
            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_MAIL_FROM, strlen(SMTP_CMD_MAIL_FROM)) == 0){

            if(smtp_state != SMTP_STATE_HELO && smtp_state != SMTP_STATE_PERIOD){
               strncat(resp, SMTP_RESP_503_ERR, sizeof(resp)-1);
            }
            else {

               if(smtp_state == SMTP_STATE_PERIOD){
                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: initiated new transaction", sdata.ttmpfile);

                  unlink(sdata.ttmpfile);

                  init_session_data(&sdata, cfg);
               }

               smtp_state = SMTP_STATE_MAIL_FROM;

               snprintf(sdata.mailfrom, SMALLBUFSIZE-1, "%s\r\n", buf);

               memset(sdata.fromemail, 0, SMALLBUFSIZE);
               extractEmail(sdata.mailfrom, sdata.fromemail);

               strncat(resp, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK));

               if((strstr(sdata.mailfrom, "MAILER-DAEMON") || strstr(sdata.mailfrom, "<>")) && strlen(cfg->our_signo) > 3) sdata.need_signo_check = 1;
            }

            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_RCPT_TO, strlen(SMTP_CMD_RCPT_TO)) == 0){

            if(smtp_state == SMTP_STATE_MAIL_FROM || smtp_state == SMTP_STATE_RCPT_TO){
               if(strlen(buf) > SMALLBUFSIZE/2){
                  strncat(resp, SMTP_RESP_550_ERR_TOO_LONG_RCPT_TO, sizeof(resp)-1);
                  continue;
               }

               if(sdata.num_of_rcpt_to < MAX_RCPT_TO-1){
                  extractEmail(buf, sdata.rcptto[sdata.num_of_rcpt_to]);
               }

               smtp_state = SMTP_STATE_RCPT_TO;

               if(cfg->blackhole_email_list[0]){
                   if(is_item_on_list(sdata.rcptto[sdata.num_of_rcpt_to], cfg->blackhole_email_list, "") == 1){
                      sdata.blackhole = 1;
                      counters.c_minefield++;

                      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: we trapped %s on the blackhole", sdata.ttmpfile, sdata.rcptto[sdata.num_of_rcpt_to]);

                      gettimeofday(&tv1, &tz);
                      store_minefield_ip(&sdata, sdata.ip);
                      gettimeofday(&tv2, &tz);
                      sdata.__minefield = tvdiff(tv2, tv1);
                   }
               }

               if(sdata.num_of_rcpt_to == 0 && (strcasestr(sdata.rcptto[0], "+ham@") || strncmp(sdata.rcptto[sdata.num_of_rcpt_to], "ham@", 4) == 0 ) ){
                  sdata.training_request = 1;
                  counters.c_fp++;
               }
     
               if(sdata.num_of_rcpt_to == 0 && (strcasestr(sdata.rcptto[0], "+spam@") || strncmp(sdata.rcptto[sdata.num_of_rcpt_to], "spam@", 5) == 0) ){
                  sdata.training_request = 1;
                  counters.c_fn++;
               }

               if(sdata.num_of_rcpt_to < MAX_RCPT_TO-1) sdata.num_of_rcpt_to++;

               strncat(resp, SMTP_RESP_250_OK, sizeof(resp)-1);
            }
            else {
               strncat(resp, SMTP_RESP_503_ERR, sizeof(resp)-1);
            }

            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_DATA, strlen(SMTP_CMD_DATA)) == 0){

            memset(last2buf, 0, sizeof(last2buf));
            memset(prevbuf, 0, sizeof(prevbuf));
            inj = ERR;
            prevlen = 0;

            if(smtp_state != SMTP_STATE_RCPT_TO){
               strncat(resp, SMTP_RESP_503_ERR, sizeof(resp)-1);
            }
            else {
               sdata.fd = open(sdata.filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP);
               if(sdata.fd == -1){
                  syslog(LOG_PRIORITY, "%s: %s", ERR_OPEN_TMP_FILE, sdata.ttmpfile);
                  strncat(resp, SMTP_RESP_451_ERR, sizeof(resp)-1);
               }
               else {
                  smtp_state = SMTP_STATE_DATA;
                  strncat(resp, SMTP_RESP_354_DATA_OK, sizeof(resp)-1);
               }

            }

            continue; 
         }


         if(strncasecmp(buf, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT)) == 0){

            smtp_state = SMTP_STATE_FINISHED;

            snprintf(buf, sizeof(buf)-1, SMTP_RESP_221_GOODBYE, cfg->hostid);
            strncat(resp, buf, sizeof(resp)-1);

            unlink(sdata.ttmpfile);
            if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);

            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_NOOP, strlen(SMTP_CMD_NOOP)) == 0){
            strncat(resp, SMTP_RESP_250_OK, sizeof(resp)-1);
            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_RESET, strlen(SMTP_CMD_RESET)) == 0){

            strncat(resp, SMTP_RESP_250_OK, sizeof(resp)-1);

            if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);
            unlink(sdata.ttmpfile);

            init_session_data(&sdata, cfg);

            smtp_state = SMTP_STATE_HELO;

            continue;
         }

         /* by default send 502 command not implemented message */

         syslog(LOG_PRIORITY, "%s: error: invalid command *%s*", sdata.ttmpfile, buf);
         strncat(resp, SMTP_RESP_502_ERR, sizeof(resp)-1);
      }


      /* now we can send our buffered response */

      if(strlen(resp) > 0){
         write1(new_sd, resp, strlen(resp), sdata.tls, data->ssl);

         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, resp);
         memset(resp, 0, sizeof(resp));

         if(starttls == 1 && sdata.tls == 0){

            if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: waiting for ssl handshake", sdata.ttmpfile);

            rc = SSL_accept(data->ssl);

            if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: SSL_accept() finished", sdata.ttmpfile);

            if(rc == 1){
               sdata.tls = 1;
            }
            else {
               ERR_error_string_n(ERR_get_error(), ssl_error, SMALLBUFSIZE);
               syslog(LOG_PRIORITY, "%s: error: SSL_accept() failed, rc=%d, errorcode: %d, error text: %s\n", sdata.ttmpfile, rc, SSL_get_error(data->ssl, rc), ssl_error);
            }
         }
      }

      if(smtp_state == SMTP_STATE_FINISHED){
         goto QUITTING;
      }

   } /* while */

   /*
    * if we are not in SMTP_STATE_QUIT and the message was not injected,
    * ie. we have timed out than send back 421 error message
    */

   if(smtp_state < SMTP_STATE_QUIT && inj == ERR){
      snprintf(buf, sizeof(buf)-1, SMTP_RESP_421_ERR, cfg->hostid);
      write1(new_sd, buf, strlen(buf), sdata.tls, data->ssl);

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

      if(sdata.fd != -1){

         syslog(LOG_PRIORITY, "%s: removing stale files: %s", sdata.ttmpfile, sdata.ttmpfile);

         close(sdata.fd);
         unlink(sdata.ttmpfile);
      }

      goto QUITTING;
   }


QUITTING:

   if(cfg->history == 0) update_counters(&sdata, data, &counters, cfg);

#ifdef NEED_MYSQL
   close_database(&sdata);
#endif

   if(sdata.tls == 1){
      SSL_shutdown(data->ssl);
      SSL_free(data->ssl);
   }

   if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "processed %llu messages", counters.c_rcvd);

   return (int)counters.c_rcvd;
}
