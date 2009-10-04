/*
 * session.c, 2009.10.04, SJ
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
#include "av.h"
#include <clapf.h>


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
   memset(sdata->xforward, 0, SMALLBUFSIZE);

   memset(sdata->clapf_id, 0, SMALLBUFSIZE);

   sdata->uid = 0;
   sdata->tot_len = 0;
   sdata->skip_id_check = 0;
   sdata->num_of_rcpt_to = 0;
   sdata->unknown_client = sdata->trapped_client = 0;
   sdata->blackhole = 0;
   sdata->need_signo_check = 0;
   sdata->training_request = 0;

   sdata->tre = '-';

   sdata->rav = AVIR_OK;

   sdata->__parsed = sdata->__av = sdata->__user = sdata->__policy = sdata->__as = sdata->__minefield = 0;
   sdata->__training = sdata->__update = sdata->__store = sdata->__inject = 0;

   sdata->spaminess = DEFAULT_SPAMICITY;

   for(i=0; i<MAX_RCPT_TO; i++) memset(sdata->rcptto[i], 0, SMALLBUFSIZE);

}


void postfix_to_clapf(int new_sd, struct __data *data, struct __config *cfg){
   int i, pos, n, inj=ERR_REJECT, state, prevlen=0;
   char *p, *q, buf[MAXBUFSIZE], puf[MAXBUFSIZE], resp[MAXBUFSIZE], prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1];
   char email[SMALLBUFSIZE], email2[SMALLBUFSIZE], virusinfo[SMALLBUFSIZE], reason[SMALLBUFSIZE];
   struct session_data sdata;
   struct _state sstate;
   struct __config my_cfg;
   int db_conn=0;
   int rc;
   struct url *a;

   struct timezone tz;
   struct timeval tv1, tv2;

   #ifdef HAVE_TRE
      size_t nmatch=0;
      char *q2;
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


#ifdef HAVE_MEMCACHED
   memcached_server_st *servers;

   sdata.memc = memcached_create(NULL);

   if(sdata.memc != NULL){
      servers = memcached_servers_parse(cfg->memcached_servers);

      if(memcached_server_push(sdata.memc, servers) != MEMCACHED_SUCCESS){
         memcached_free(sdata.memc);
         sdata.memc = NULL;
      }

      memcached_server_list_free(servers);
   }
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

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: period: *%s*", sdata.ttmpfile, puf+pos);


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
               gettimeofday(&tv1, &tz);

               sstate = parse_message(sdata.ttmpfile, &sdata, cfg);

               gettimeofday(&tv2, &tz);
               sdata.__parsed = tvdiff(tv2, tv1);


               if(sstate.has_base64 == 0 && cfg->always_scan_message == 0) sdata.need_scan = 0;
               else sdata.need_scan = 1;


               /* do antivirus check, if we have to */

            #ifdef HAVE_ANTIVIRUS
               gettimeofday(&tv1, &tz);
               #ifdef HAVE_LIBCLAMAV
                  sdata.rav = do_av_check(&sdata, email, email2, &virusinfo[0], data->engine, cfg);
               #else
                  sdata.rav = do_av_check(&sdata, email, email2, &virusinfo[0], cfg);
               #endif
               gettimeofday(&tv2, &tz);
               sdata.__av = tvdiff(tv2, tv1);
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


            #ifdef HAVE_LMTP
               for(i=0; i<sdata.num_of_rcpt_to; i++){
            #else
               i = 0;
            #endif
                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: round %d in injection", sdata.ttmpfile, i);

                  extract_email(sdata.rcptto[i], email);

               #ifdef HAVE_ANTISPAM
                  if(db_conn == 1){
                     if(process_message(&sdata, &sstate, data, email, email2, cfg, &my_cfg) == 0){
                        /* if we have to discard the message */
                        goto SEND_RESULT;
                     }
                  }


                  /* inject message back */

                  gettimeofday(&tv1, &tz);

                  if(sdata.spaminess >= my_cfg.spam_overall_limit){

                    /* shall we redirect the message into oblivion? 2007.02.07, SJ */
                    if(sdata.spaminess >= my_cfg.spaminess_oblivion_limit)
                       inj = ERR_DROP_SPAM;
                    else
                       inj = inject_mail(&sdata, i, cfg->spam_smtp_addr, cfg->spam_smtp_port, sdata.spaminessbuf, &resp[0], &my_cfg, NULL);
                  }
                  else {
                     inj = inject_mail(&sdata, i, cfg->postfix_addr, cfg->postfix_port, sdata.spaminessbuf, &resp[0], &my_cfg, NULL);
                  }

                  gettimeofday(&tv2, &tz);
                  sdata.__inject = tvdiff(tv2, tv1);

               #else
                  gettimeofday(&tv1, &tz);

                  inj = inject_mail(&sdata, i, cfg->postfix_addr, cfg->postfix_port, NULL, &resp[0], &my_cfg, NULL);

                  gettimeofday(&tv2, &tz);
                  sdata.__inject = tvdiff(tv2, tv1);
               #endif

                  /* set the accept buffer */

                  if(inj == OK || inj == ERR_DROP_SPAM){
                     snprintf(sdata.acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                  }
                  else if(inj == ERR_REJECT){
                     snprintf(sdata.acceptbuf, MAXBUFSIZE-1, "550 %s <%s>\r\n", sdata.ttmpfile, email);
                  }
                  else {
                     snprintf(sdata.acceptbuf, MAXBUFSIZE-1, "451 %s <%s>\r\n", sdata.ttmpfile, email);
                  }

            #ifdef HAVE_ANTISPAM
               SEND_RESULT:
            #endif
                  send(new_sd, sdata.acceptbuf, strlen(sdata.acceptbuf), 0);

                  if(inj == ERR_DROP_SPAM) syslog(LOG_PRIORITY, "%s: dropped spam", sdata.ttmpfile);
                  else if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, sdata.acceptbuf);



                  /* syslog at the end */

                  snprintf(reason, SMALLBUFSIZE-1, "delay=%.2f, delays=%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f", 
                               (sdata.__parsed+sdata.__av+sdata.__user+sdata.__policy+sdata.__minefield+sdata.__as+sdata.__training+sdata.__update+sdata.__store+sdata.__inject)/1000000.0,
                                   sdata.__parsed/1000000.0, sdata.__av/1000000.0, sdata.__user/1000000.0, sdata.__policy/1000000.0, sdata.__minefield/1000000.0, sdata.__as/1000000.0,
                                       sdata.__training/1000000.0, sdata.__update/1000000.0, sdata.__store/1000000.0, sdata.__inject/1000000.0);

                  if(sdata.spaminess >= cfg->spam_overall_limit){
                     syslog(LOG_PRIORITY, "%s: %s got SPAM, %.4f, %d, relay=%s:%d, %s, status=%s", sdata.ttmpfile, email, sdata.spaminess, sdata.tot_len, my_cfg.spam_smtp_addr, my_cfg.spam_smtp_port, reason, resp);
                  } else if(sdata.rav == AVIR_VIRUS) {
                     syslog(LOG_PRIORITY, "%s: %s got VIRUS (%s), %.4f, %d, relay=%s:%d, %s, status=%s", sdata.ttmpfile, email, virusinfo, sdata.spaminess, sdata.tot_len, my_cfg.postfix_addr, my_cfg.postfix_port, reason, resp);
                  } else {
                     syslog(LOG_PRIORITY, "%s: %s got HAM, %.4f, %d, relay=%s:%d, %s, status=%s", sdata.ttmpfile, email, sdata.spaminess, sdata.tot_len, my_cfg.postfix_addr, my_cfg.postfix_port, reason, resp);
                  }

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
                  to handle the additional commands, such as QUIT */

               /* if we miss the trailing \r\n, ie. we need another read */

               if(puf[n-2] != '\r' && puf[n-1] != '\n'){
                  memmove(puf, puf+pos, n-pos);
                  memset(puf+n-pos, 0, MAXBUFSIZE-n+pos);
                  i = recvtimeout(new_sd, buf, MAXBUFSIZE, 0);
                  strncat(puf, buf, MAXBUFSIZE-1-n+pos);
                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: partial read: %s", sdata.ttmpfile, puf);
                  pos = 0;
               }

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

         // EHLO

         if(strncasecmp(buf, SMTP_CMD_EHLO, strlen(SMTP_CMD_EHLO)) == 0 || strncasecmp(buf, LMTP_CMD_LHLO, strlen(LMTP_CMD_LHLO)) == 0){
            if(state == SMTP_STATE_INIT) state = SMTP_STATE_HELO;

            snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_250_EXTENSIONS, cfg->hostid);
            strncat(resp, buf, MAXBUFSIZE-1);

            continue;

            /* FIXME: implement the ENHANCEDSTATUSCODE extensions */
         }



         // HELO, let's play it simple for kids and grandmas ...

         if(strncasecmp(buf, SMTP_CMD_HELO, strlen(SMTP_CMD_HELO)) == 0){
            if(state == SMTP_STATE_INIT) state = SMTP_STATE_HELO;

            strncat(resp, SMTP_RESP_250_OK, MAXBUFSIZE-1);

            continue;
         }



         // XFORWARD

         if(strncasecmp(buf, SMTP_CMD_XFORWARD, strlen(SMTP_CMD_XFORWARD)) == 0){

            /*
             * XFORWARD NAME=83-131-14-231.adsl.net.t-com.hr ADDR=83.131.14.231..
             * XFORWARD PROTO=SMTP HELO=rhwfsvji..
             */

            if(strlen(sdata.xforward) + strlen(buf) < SMALLBUFSIZE-2){
               strncat(sdata.xforward, buf, SMALLBUFSIZE-3);
               strncat(sdata.xforward, "\r\n", SMALLBUFSIZE-1);
            }

            trim(buf);

            /* extract client name */

         #ifdef HAVE_TRE
            i = 0;

            gettimeofday(&tv1, &tz);

            q = strstr(buf, "NAME=");
            if(q){
               q2 = strchr(q+5, ' ');
               if(q2) *q2 = '\0';

               while(i < data->n_regex && sdata.tre != '+'){
                  if(regexec(&(data->pregs[i]), q, nmatch, NULL, 0) == 0) sdata.tre = '+';

                  i++;
               }

               if(q2) *q2 = ' ';

               gettimeofday(&tv2, &tz);
               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: zombie check: %c [%d] %s in %ld us", sdata.ttmpfile, sdata.tre, i, q, tvdiff(tv2, tv1));
            }
         #endif

            /* extract client address */

            q = strstr(buf, "ADDR=");
            if(q){
               snprintf(sdata.client_addr, IPLEN-1, "%s", q+5);
               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: client address: %s", sdata.ttmpfile, sdata.client_addr);
            }

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
                  snprintf(sdata.rcptto[sdata.num_of_rcpt_to], SMALLBUFSIZE-1, "%s\r\n", buf);
                  sdata.num_of_rcpt_to++;
               }

               state = SMTP_STATE_RCPT_TO;

               /* check against blackhole addresses */

               extract_email(buf, email);

               if(data->blackhole){
                  a = data->blackhole;

                  while(a){
                     if(strcmp(a->url_str, email) == 0){
                        if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: we have %s on the blacklist", sdata.ttmpfile, email);
                        sdata.blackhole = 1;

                     #ifdef HAVE_BLACKHOLE
                        gettimeofday(&tv1, &tz);
                        store_minefield_ip(&sdata, cfg);

                        gettimeofday(&tv2, &tz);
                        sdata.__minefield = tvdiff(tv2, tv1);
                     #endif

                        break;
                     }
                     a = a->r;
                  }
               }


               /* is it a training request? */

               if(sdata.num_of_rcpt_to == 1 && (strcasestr(sdata.rcptto[0], "+spam@") || strcasestr(sdata.rcptto[0], "+ham@") || strncmp(email, "spam@", 5) == 0 || strncmp(email, "ham@", 4) == 0 ) ){
                  sdata.training_request = 1;
               }


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
         syslog(LOG_PRIORITY, "%s: invalid command: %s", sdata.ttmpfile, buf);

         strncat(resp, SMTP_RESP_502_ERR, MAXBUFSIZE-1);
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
#ifdef HAVE_MEMCACHED
   if(sdata.memc != NULL) memcached_free(sdata.memc);
#endif

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "child has finished");

   _exit(0);
}
