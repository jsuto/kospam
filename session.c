/*
 * session.c, 2007.06.13, SJ
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
#include "misc.h"
#include "smtpcodes.h"
#include "errmsg.h"
#include "mime.h"
#include "avg.h"
#include "avast.h"
#include "kav.h"
#include "drweb.h"
#include "clamd.h"
#include "session.h"
#include "messages.h"
#include "config.h"

int sd, inj, ret, rav, prevlen=0;
char prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1];
struct timezone tz;
struct timeval tv_start, tv_rcvd, tv_scnd, tv_sent, tv_stop;
struct session_data sdata;

int inject_mail(struct session_data sdata, char *smtpaddr, int smtpport, char *spaminessbuf, struct __config cfg, char *notify);

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   int update_training_metadata(char *tmpfile, char rcptto[MAX_RCPT_TO][MAXBUFSIZE], int num_of_rcpt_to, struct __config cfg);
#endif


/*
 * kill child if it works too long or is frozen
 */

void kill_child(){
   syslog(LOG_PRIORITY, "child is killed by force");
   unlink(sdata.ttmpfile);
   exit(0);
}


/*
 * syslog ham/spam status per email addresses
 */

void log_ham_spam_per_email(struct session_data sdata, int ham_or_spam){
   int i;
   char *p, *q, a[MAXBUFSIZE];

   for(i=0; i<sdata.num_of_rcpt_to; i++){

       snprintf(a, MAXBUFSIZE-1, sdata.rcptto[i]);
       p = strchr(a, '<');
       if(p){
          p++;
          q = strchr(p, '>');
          if(q)
             *q = '\0';
       }
       else
          p = a;

       if(ham_or_spam == 0)
          syslog(LOG_PRIORITY, "%s: %s got HAM", sdata.ttmpfile, p);
       else
          syslog(LOG_PRIORITY, "%s: %s got SPAM", sdata.ttmpfile, p);
   }
}


/*
 * init SMTP session
 */

void init_child(){
   int i;

   memset(sdata.mailfrom, 0, MAXBUFSIZE);
   memset(last2buf, 0, 2*MAXBUFSIZE+1);
   memset(prevbuf, 0, MAXBUFSIZE);

   inj = ERR_REJECT;

   sdata.uid = 0;
   sdata.tot_len = 0;
   prevlen = 0;
   sdata.num_of_rcpt_to = 0;

   for(i=0; i<MAX_RCPT_TO; i++){
      memset(sdata.rcptto[i], 0, MAXBUFSIZE);
   }

   gettimeofday(&tv_start, &tz);

   make_rnd_string(&(sdata.ttmpfile[0]));

   unlink(sdata.ttmpfile);
}

#ifdef HAVE_LIBCLAMAV
   void postfix_to_clapf(int new_sd, struct __config cfg, struct cl_limits limits, struct cl_node *root){
#else
   void postfix_to_clapf(int new_sd, struct __config cfg){
#endif

   int n, state, fd;
   char buf[MAXBUFSIZE], acceptbuf[MAXBUFSIZE], queuedfile[SMALLBUFSIZE];

   #ifdef HAVE_ANTIVIRUS
      char virusinfo[SMALLBUFSIZE];
   #endif

   #ifdef HAVE_LIBCLAMAV
      const char *virname;
   #endif

   #ifdef HAVE_ANTISPAM
      double spaminess=DEFAULT_SPAMICITY;
      char spamfile[MAXBUFSIZE], spaminessbuf[MAXBUFSIZE], reason[SMALLBUFSIZE];
      struct timeval tv_spam_start, tv_spam_stop;
   #endif

   #ifdef HAVE_AVG
      int i;
      struct rfc822_attachment Qmime;
      char mimefile[SMALLBUFSIZE];
   #endif

   alarm(cfg.session_timeout);
   signal(SIGALRM, kill_child);

   state = SMTP_STATE_INIT;

   init_child();

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: fork()", sdata.ttmpfile);

   fd = open(sdata.ttmpfile, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
   if(fd == -1){
       syslog(LOG_PRIORITY, "%s: %s", ERR_OPEN_TMP_FILE, sdata.ttmpfile);
       send(new_sd, SMTP_RESP_421_ERR_TMP, strlen(SMTP_RESP_421_ERR_TMP), 0);
       _exit(0);
   }

   // send 220 SMTP banner

   send(new_sd, SMTP_RESP_220_BANNER, strlen(SMTP_RESP_220_BANNER), 0);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_220_BANNER);

   while((n = recvtimeout(new_sd, buf, MAXBUFSIZE, 0)) > 0){

      // HELO/EHLO

      if(strncasecmp(buf, SMTP_CMD_HELO, strlen(SMTP_CMD_HELO)) == 0 || strncasecmp(buf, SMTP_CMD_EHLO, strlen(SMTP_CMD_EHLO)) == 0){
         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            if(state == SMTP_STATE_INIT)
               state = SMTP_STATE_HELO;
            send(new_sd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);

            continue;
         }

         // MAIL FROM

         if(strncasecmp(buf, SMTP_CMD_MAIL_FROM, strlen(SMTP_CMD_MAIL_FROM)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            if(state != SMTP_STATE_HELO){
               send(new_sd, SMTP_RESP_503_ERR, strlen(SMTP_RESP_503_ERR), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_503_ERR);
            } 
            else {
               state = SMTP_STATE_MAIL_FROM;
               memcpy(sdata.mailfrom, buf, MAXBUFSIZE-1);
               send(new_sd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);
            }

            continue;
         }

         // RCPT TO

         if(strncasecmp(buf, SMTP_CMD_RCPT_TO, strlen(SMTP_CMD_RCPT_TO)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            if(state == SMTP_STATE_MAIL_FROM || state == SMTP_STATE_RCPT_TO){
               if(sdata.num_of_rcpt_to < MAX_RCPT_TO){
                  memcpy(sdata.rcptto[sdata.num_of_rcpt_to], buf, MAXBUFSIZE-1);
                  sdata.num_of_rcpt_to++;
               }

               state = SMTP_STATE_RCPT_TO;
               send(new_sd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);
            }
            else {
               send(new_sd, SMTP_RESP_503_ERR, strlen(SMTP_RESP_503_ERR), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_503_ERR);
            }

            continue;
         }

         // DATA

         if(strncasecmp(buf, SMTP_CMD_DATA, strlen(SMTP_CMD_DATA)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            if(state != SMTP_STATE_RCPT_TO){
               send(new_sd, SMTP_RESP_503_ERR, strlen(SMTP_RESP_503_ERR), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_503_ERR);
            }
            else {
               state = SMTP_STATE_DATA;
               send(new_sd, SMTP_RESP_354_DATA_OK, strlen(SMTP_RESP_354_DATA_OK), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_354_DATA_OK);
            }

            continue;
         }

         // QUIT

         if(strncasecmp(buf, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            state = SMTP_STATE_FINISHED;
            send(new_sd, SMTP_RESP_221_GOODBYE, strlen(SMTP_RESP_221_GOODBYE), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_221_GOODBYE);

            goto QUITTING;
         }

         // NOOP

         if(strncasecmp(buf, SMTP_CMD_NOOP, strlen(SMTP_CMD_NOOP)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            send(new_sd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);

            continue;
         }

         // RSET

         if(strncasecmp(buf, SMTP_CMD_RESET, strlen(SMTP_CMD_RESET)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            init_child();

            state = SMTP_STATE_HELO;

            /* recreate file, 2006.01.03, SJ */

            fd = open(sdata.ttmpfile, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
            if(fd == -1){
               syslog(LOG_PRIORITY, "%s: %s", ERR_OPEN_TMP_FILE, sdata.ttmpfile);
               send(new_sd, SMTP_RESP_421_ERR_TMP, strlen(SMTP_RESP_421_ERR_TMP), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_421_ERR_TMP);
               _exit(0);
            }

            send(new_sd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);

            continue;
         }

         /* accept mail data */

         if(state == SMTP_STATE_DATA){
            write(fd, buf, n);
            sdata.tot_len += n;

            /* join the last 2 buffer, 2004.08.30, SJ */

            memset(last2buf, 0, 2*MAXBUFSIZE+1);
            memcpy(last2buf, prevbuf, MAXBUFSIZE);
            memcpy(last2buf+prevlen, buf, MAXBUFSIZE);

            if(search_in_buf(last2buf, 2*MAXBUFSIZE+1, SMTP_CMD_PERIOD, 5) == 1){

               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: (.)", sdata.ttmpfile);

               state = SMTP_STATE_PERIOD;
               close(fd);


         #ifdef HAVE_ANTIVIRUS

               /* antivirus result is ok by default, 2006.02.08, SJ */
               rav = AVIR_OK;

               gettimeofday(&tv_rcvd, &tz);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: virus scanning", sdata.ttmpfile);

            #ifdef HAVE_LIBCLAMAV

               /* whether to mark archives as viruses if maxfiles, maxfilesize, or maxreclevel limit is reached, 2006.02.16, SJ */

               if(cfg.use_libclamav_block_max_feature == 1)
                  ret = cl_scanfile(sdata.ttmpfile, &virname, NULL, root, &limits,
                       CL_SCAN_STDOPT | CL_SCAN_ARCHIVE | CL_SCAN_BLOCKENCRYPTED | CL_SCAN_BLOCKMAX | CL_SCAN_MAIL | CL_SCAN_OLE2);
               else
                  ret = cl_scanfile(sdata.ttmpfile, &virname, NULL, root, &limits,
                       CL_SCAN_STDOPT | CL_SCAN_ARCHIVE | CL_SCAN_BLOCKENCRYPTED | CL_SCAN_MAIL | CL_SCAN_OLE2);



               if(ret == CL_VIRUS){
                  memset(virusinfo, 0, SMALLBUFSIZE);
                  strncpy(virusinfo, virname, SMALLBUFSIZE-1);
                  rav = AVIR_VIRUS;
               }

            #endif


            #ifdef HAVE_AVG

               /* extract attachments from message file */

               Qmime = extract_from_rfc822(sdata.ttmpfile);
               if(Qmime.result == 1){

                  if(Qmime.cnt > 0){

                     /* scan directory */

                     if(avg_scan(cfg.avg_addr, cfg.avg_port, cfg.workdir, Qmime.tmpdir, sdata.ttmpfile, cfg.verbosity, virusinfo) == AVG_VIRUS)
                        rav = AVIR_VIRUS;

                     /* and remove files */

                     for(i=1; i<=Qmime.cnt; i++){
                        snprintf(mimefile, SMALLBUFSIZE-1, "%s/%d", Qmime.tmpdir, i);
                        if(unlink(mimefile) == -1)
                           syslog(LOG_PRIORITY, "%s: failed to remove temp file %s", sdata.ttmpfile, mimefile);
                     }
                  }

                  /* remove created temporary directory */

                  if(rmdir(Qmime.tmpdir) == -1)
                     syslog(LOG_PRIORITY, "%s: failed to remove temp dir %s", sdata.ttmpfile, Qmime.tmpdir);

               }
               else
                  syslog(LOG_PRIORITY, "%s: internal error while extracting", sdata.ttmpfile);

            #endif



            #ifdef HAVE_AVAST
               if(avast_scan(cfg.avast_addr, cfg.avast_port, cfg.workdir, sdata.ttmpfile, cfg.verbosity, virusinfo) == AVAST_VIRUS)
                  rav = AVIR_VIRUS;
            #endif


            #ifdef HAVE_KAV
               if(kav_scan(cfg.kav_socket, cfg.workdir, sdata.ttmpfile, cfg.verbosity, virusinfo) == KAV_VIRUS)
                  rav = AVIR_VIRUS;
            #endif

            #ifdef HAVE_DRWEB
               if(drweb_scan(cfg.drweb_socket, sdata.ttmpfile, cfg.verbosity, virusinfo) == DRWEB_VIRUS)
                  rav = AVIR_VIRUS;
            #endif

            #ifdef HAVE_CLAMD
               chmod(sdata.ttmpfile, 0644);
               if(clamd_scan(cfg.clamd_socket, cfg.workdir, sdata.ttmpfile, cfg.verbosity, virusinfo) == CLAMD_VIRUS)
                  rav = AVIR_VIRUS;
            #endif

               gettimeofday(&tv_scnd, &tz);
               memset(acceptbuf, 0, MAXBUFSIZE);

               if(rav == AVIR_VIRUS){
                  snprintf(acceptbuf, MAXBUFSIZE-1, "%s Virus found in your mail (%s), id: %s\r\n", SMTP_RESP_550_ERR_PREF, virusinfo, sdata.ttmpfile);

                  syslog(LOG_PRIORITY, "%s found in %s, rcvd: %ld [ms], scnd: %ld [ms], len: %d",
                       virusinfo, sdata.ttmpfile, tvdiff(tv_rcvd, tv_start)/1000, tvdiff(tv_scnd, tv_rcvd)/1000, sdata.tot_len);


                  /* move infected mail to quarantine, 2006.02.16, SJ */

                  if(strlen(cfg.quarantine_dir) > 3)
                     move_message_to_quarantine(sdata.ttmpfile, cfg.quarantine_dir, sdata.mailfrom, sdata.rcptto, sdata.num_of_rcpt_to);

                  /* send notification if cfg.localpostmaster is set, 2005.10.04, SJ */

                  if(strlen(cfg.clapfemail) > 3 && strlen(cfg.localpostmaster) > 3){

                     snprintf(buf, MAXBUFSIZE-1, "From: <%s>\r\nTo: <%s>\r\nSubject: %s has been infected\r\n\r\n"
                                    "E-mail from %s to %s (id: %s) was infected\r\n\r\n.\r\n",
                                     cfg.clapfemail, cfg.localpostmaster, sdata.ttmpfile, sdata.mailfrom, sdata.rcptto[0], sdata.ttmpfile);

                     snprintf(sdata.rcptto[0], MAXBUFSIZE-1, "RCPT TO: <%s>\r\n", cfg.localpostmaster);
                     sdata.num_of_rcpt_to = 1;
                     ret = inject_mail(sdata, cfg.postfix_addr, cfg.postfix_port, NULL, cfg, buf);
                     if(ret == 0)
                        syslog(LOG_PRIORITY, "notification about %s to %s failed", sdata.ttmpfile, cfg.localpostmaster);
                  }

               }
               else {
         #endif /* HAVE_ANTIVIRUS */

         #ifdef HAVE_ANTISPAM

                    /* skip spam test if the message size is above max_message_size_to_filter, 2005.12.08, SJ */

                    if(cfg.use_antispam == 1 && (cfg.max_message_size_to_filter == 0 || sdata.tot_len < cfg.max_message_size_to_filter) ){
                       if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running Bayesian test", sdata.ttmpfile);

                       memset(trainbuf, 0, SMALLBUFSIZE);
                       memset(spamfile, 0, MAXBUFSIZE);
                       snprintf(spamfile, MAXBUFSIZE-1, "%s/%s", cfg.workdir, sdata.ttmpfile);

                       gettimeofday(&tv_spam_start, &tz);

                       spaminess = bayes_file(spamfile, sdata, cfg);

                       if(spaminess >= ERR_BAYES_NO_SPAM_FILE)
                          syslog(LOG_PRIORITY, "%s: Error happened while Bayesian test (%.2f)", sdata.ttmpfile, spaminess);

                       gettimeofday(&tv_spam_stop, &tz);

                       syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, spaminess, sdata.tot_len, tvdiff(tv_spam_stop, tv_spam_start)/1000);

                       if(spaminess >= cfg.spam_overall_limit && spaminess < ERR_BAYES_NO_SPAM_FILE){
                          memset(reason, 0, SMALLBUFSIZE);

                          if(spaminess == cfg.spaminess_of_strange_language_stuff) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_STRANGE_LANGUAGE);
                          if(spaminess == cfg.spaminess_of_too_much_spam_in_top15) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_TOO_MUCH_SPAM_IN_TOP15);
                          if(spaminess == cfg.spaminess_of_blackholed_mail) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_BLACKHOLED);
                          if(spaminess == cfg.spaminess_of_caught_by_surbl) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_CAUGHT_BY_SURBL);
                          if(spaminess == cfg.spaminess_of_text_and_base64) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_TEXT_AND_BASE64);
                          if(spaminess == cfg.spaminess_of_embed_image) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_EMBED_IMAGE);
                          if(spaminess > 0.9999) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_ABSOLUTELY_SPAM);

                          /* add additional headers, credits: Mariano, 2006.08.14 */

                          snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s%s%s\r\n",
                             cfg.clapf_header_field, spaminess, cfg.clapf_header_field, sdata.ttmpfile, reason, trainbuf, cfg.clapf_spam_header_field);


                          log_ham_spam_per_email(sdata, 1);
                       }
                       else {
                          snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s", cfg.clapf_header_field, spaminess, cfg.clapf_header_field, sdata.ttmpfile, trainbuf);

                          log_ham_spam_per_email(sdata, 0);
                       }

                    }
                    else {
                       snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, sdata.ttmpfile);
                    }

         #endif
                    gettimeofday(&tv_stop, &tz);

                    snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok: queued as %s, rcvd: %ld [ms], scnd: %ld [ms], total: %ld [ms], len: %d\r\n",
                        sdata.ttmpfile, tvdiff(tv_rcvd, tv_start)/1000, tvdiff(tv_scnd, tv_rcvd)/1000, tvdiff(tv_stop, tv_start)/1000, sdata.tot_len);


                    gettimeofday(&tv_sent, &tz);

         #ifdef HAVE_ANTISPAM
                    /*
                       forward spam to the SMTP server defined by spam_smtp_addr and spam_smtp_port
                       or pass it back to our postfix, 2006.06.30, SJ
                     */

                    if(spaminess >= cfg.spam_overall_limit && spaminess < ERR_BAYES_NO_SPAM_FILE){
                       /* shall we redirect the message into oblivion? 2007.02.07, SJ */
                       if(spaminess >= cfg.spaminess_oblivion_limit)
                          inj = ERR_DROP_SPAM;
                       else
                          inj = inject_mail(sdata, cfg.spam_smtp_addr, cfg.spam_smtp_port, spaminessbuf, cfg, NULL);
                    }
                    else
                       inj = inject_mail(sdata, cfg.postfix_addr, cfg.postfix_port, spaminessbuf, cfg, NULL);


                 #ifdef HAVE_MYSQL_TOKEN_DATABASE
                    /* insert meta data to queue table, 2007.05.16, SJ */
                    update_training_metadata(sdata.ttmpfile, sdata.rcptto, sdata.num_of_rcpt_to, cfg);
                 #endif

         #else
                    inj = inject_mail(sdata, cfg.postfix_addr, cfg.postfix_port, NULL, cfg, NULL);
         #endif

                    /* message was injected back */

                    gettimeofday(&tv_stop, &tz);

                    if(inj == OK){
                        snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok: queued as %s, rcvd: %ld [ms], scnd: %ld [ms], sent: %ld [ms], total: %ld [ms], len: %d\r\n",
                           sdata.ttmpfile, tvdiff(tv_rcvd, tv_start)/1000, tvdiff(tv_scnd, tv_rcvd)/1000, tvdiff(tv_stop, tv_sent)/1000, tvdiff(tv_stop, tv_start)/1000, sdata.tot_len);

                    }

                    /* message was rejected */

                    else if(inj == ERR_REJECT){
                       snprintf(acceptbuf, MAXBUFSIZE-1, "550 %s rejected\r\n", sdata.ttmpfile);
                    }

                    /* this is spam to be dropped, 2007.02.07, SJ */

                    else if(inj == ERR_DROP_SPAM){
                       snprintf(acceptbuf, MAXBUFSIZE-1, "550 %s dropping spam\r\n", sdata.ttmpfile);
                    }

                    /* we were not able to inject the message back */

                    else {
                       snprintf(acceptbuf, MAXBUFSIZE-1, "%s", SMTP_RESP_451_ERR);
                    }

         #ifdef HAVE_ANTIVIRUS
                 }
         #endif

               send(new_sd, acceptbuf, strlen(acceptbuf), 0);

            } /* PERIOD found */

            memcpy(prevbuf, buf, n);
            prevlen = n;

            continue;

         } /* SMTP DATA */


      /* by default send 502 command not implemented message */

      send(new_sd, SMTP_RESP_502_ERR, strlen(SMTP_RESP_502_ERR), 0);
      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_502_ERR);

   } /* while */

   /*
    * if we are not in SMTP_STATE_QUIT and the message was not injected,
    * ie. we have timed out than send back 421 error message, 2005.12.29, SJ
    */

   if(state < SMTP_STATE_QUIT && inj != OK){
      send(new_sd, SMTP_RESP_421_ERR, strlen(SMTP_RESP_421_ERR), 0);
      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_421_ERR);

      goto QUITTING;
   }


QUITTING:

   close(fd);

   if(strlen(cfg.queuedir) > 2){
      snprintf(queuedfile, SMALLBUFSIZE-1, "%s/%s", cfg.queuedir, sdata.ttmpfile);
      link(sdata.ttmpfile, queuedfile);
   }

   unlink(sdata.ttmpfile);
   syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);

   _exit(0);

}
