/*
 * session.c, 2007.10.09, SJ
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
#include "sql.h"
#include "config.h"

int sd, inj, ret, rav, prevlen=0;
char prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1];
struct timezone tz;
struct timeval tv_start, tv_rcvd, tv_scnd, tv_sent, tv_stop, tv_meta1, tv_meta2;
struct session_data sdata;
int x;

int inject_mail(struct session_data sdata, int msg, char *smtpaddr, int smtpport, char *spaminessbuf, struct __config cfg, char *notify);

#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL mysql;
   int mysql_connection=0;
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   int rc;
#endif


/*
 * kill child if it works too long or is frozen
 */

void kill_child(){
   syslog(LOG_PRIORITY, "%s: child is killed by force", sdata.ttmpfile);
   unlink(sdata.ttmpfile);
   exit(0);
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

   int i, n, state, fd;
   char *p, buf[MAXBUFSIZE], acceptbuf[MAXBUFSIZE], queuedfile[SMALLBUFSIZE], email[SMALLBUFSIZE];

   #ifdef HAVE_ANTIVIRUS
      char virusinfo[SMALLBUFSIZE];
   #endif

   #ifdef HAVE_LIBCLAMAV
      const char *virname;
   #endif

   #ifdef HAVE_ANTISPAM
      double spaminess=DEFAULT_SPAMICITY;
      char spamfile[MAXBUFSIZE], spaminessbuf[MAXBUFSIZE], reason[SMALLBUFSIZE], qpath[SMALLBUFSIZE];
      struct timeval tv_spam_start, tv_spam_stop;
      struct _state sstate;
      struct ue UE;
      int is_spam;
   #endif

   #ifdef HAVE_AVG
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
       snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_421_ERR_TMP, cfg.hostid);
       send(new_sd, buf, strlen(buf), 0);
       _exit(0);
   }

   // send 220 LMTP banner

   snprintf(buf, MAXBUFSIZE-1, LMTP_RESP_220_BANNER, cfg.hostid);
   send(new_sd, buf, strlen(buf), 0);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

   while((n = recvtimeout(new_sd, buf, MAXBUFSIZE, 0)) > 0){

      // HELO/EHLO

      if(strncasecmp(buf, SMTP_CMD_HELO, strlen(SMTP_CMD_HELO)) == 0 || strncasecmp(buf, SMTP_CMD_EHLO, strlen(SMTP_CMD_EHLO)) == 0 || strncasecmp(buf, LMTP_CMD_LHLO, strlen(LMTP_CMD_LHLO)) == 0){
         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            if(state == SMTP_STATE_INIT) state = SMTP_STATE_HELO;

            send(new_sd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);


            /* FIXME: implement the ENHANCEDSTATUSCODE and the PIPELINING extensions */



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

            snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_221_GOODBYE, cfg.hostid);
            send(new_sd, buf, strlen(buf), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

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

            /* remove old queue file, 2007.07.17, SJ */

            syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);
            unlink(sdata.ttmpfile);

            init_child();

            state = SMTP_STATE_HELO;


            /* recreate file, 2006.01.03, SJ */

            fd = open(sdata.ttmpfile, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
            if(fd == -1){
               syslog(LOG_PRIORITY, "%s: %s", ERR_OPEN_TMP_FILE, sdata.ttmpfile);

               snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_421_ERR_TMP, cfg.hostid);
               send(new_sd, buf, strlen(buf), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

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
               if(clamd_scan(cfg.clamd_socket, cfg.chrootdir, cfg.workdir, sdata.ttmpfile, cfg.verbosity, virusinfo) == CLAMD_VIRUS)
                  rav = AVIR_VIRUS;
            #endif

               gettimeofday(&tv_scnd, &tz);

               if(rav == AVIR_VIRUS){
                  syslog(LOG_PRIORITY, "%s: Virus found %s", sdata.ttmpfile, virusinfo);

                  /* FIXME: move quarantine here */
               }

         #endif /* HAVE_ANTIVIRUS */


               /* open database backend handler */

            #ifdef HAVE_MYSQL
               mysql_init(&mysql);
               if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
                  mysql_connection = 1;
               else {
                  mysql_connection = 0;
                  syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_MYSQL_CONNECT);
               }
            #endif
            #ifdef HAVE_SQLITE3
                rc = sqlite3_open(cfg.sqlite3, &db);
                if(rc){
                   syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_SQLITE3_OPEN);
                }
            #endif

            #ifdef HAVE_ANTISPAM
                sstate = parse_message(sdata.ttmpfile, cfg);
            #endif

               /* send results back to the '.' command */

               for(i=0; i<sdata.num_of_rcpt_to; i++){
                  memset(acceptbuf, 0, MAXBUFSIZE);
                  memset(email, 0, SMALLBUFSIZE);

                  p = strchr(sdata.rcptto[i], '<');
                  if(p){
                     snprintf(email, SMALLBUFSIZE-1, "%s", p+1);
                     p = strchr(email, '>');
                     if(p) *p = '\0';
                  }

                  if(rav == AVIR_VIRUS){
                     snprintf(acceptbuf, MAXBUFSIZE-1, "%s <%s>\r\n", SMTP_RESP_550_ERR_PREF, email);

                     if(cfg.silently_discard_infected_email == 1)
                        snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                     else
                        snprintf(acceptbuf, MAXBUFSIZE-1, "550 %s %s\r\n", sdata.ttmpfile, email);

                     goto SEND_RESULT;
                  }
               #ifdef HAVE_ANTISPAM
                  is_spam = 0;

                  /* run statistical antispam check */

                  if(cfg.use_antispam == 1 && (cfg.max_message_size_to_filter == 0 || sdata.tot_len < cfg.max_message_size_to_filter) ){
                     if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running Bayesian test", sdata.ttmpfile);

                     memset(trainbuf, 0, SMALLBUFSIZE);
                     memset(spamfile, 0, MAXBUFSIZE);
                     snprintf(spamfile, MAXBUFSIZE-1, "%s/%s", cfg.workdir, sdata.ttmpfile);

                     gettimeofday(&tv_spam_start, &tz);


                  #ifdef HAVE_MYSQL
                     if(mysql_connection == 1){
                        UE = get_user_from_email(mysql, email);
                        sdata.uid = UE.uid;

                        /* if we have forwarded something for retraining */

                        if(sdata.num_of_rcpt_to == 1 && (str_case_str(sdata.rcptto[0], "+spam@") || str_case_str(sdata.rcptto[0], "+ham@")) ){
                           snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                           retraining(mysql, sdata, UE.name, cfg);
                           goto SEND_RESULT;
                        }
                        else
                           spaminess = bayes_file(mysql, spamfile, sstate, sdata, cfg);


                        gettimeofday(&tv_spam_stop, &tz);
                     }
                     else {
                        gettimeofday(&tv_spam_stop, &tz);
                        spaminess = DEFAULT_SPAMICITY;
                     }
                  #endif
                  #ifdef HAVE_SQLITE3
                     if(rc){
                        spaminess = DEFAULT_SPAMICITY;
                        gettimeofday(&tv_spam_stop, &tz);
                     }
                     else {
                        UE = get_user_from_email(db, email);
                        sdata.uid = UE.uid;

                        /* if we have forwarded something for retraining */

                        if(sdata.num_of_rcpt_to == 1 && (str_case_str(sdata.rcptto[0], "+spam@") || str_case_str(sdata.rcptto[0], "+ham@")) ){
                           snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                           retraining(db, sdata, UE.name, cfg);
                           goto SEND_RESULT;
                        }
                        else
                           spaminess = bayes_file(db, spamfile, sstate, sdata, cfg);

                        gettimeofday(&tv_spam_stop, &tz);
                     }
                  #endif

                     /* rename file name according to its spamicity status, 2007.10.04, SJ */

                     if(cfg.store_metadata == 1 && UE.name){
                        if(spaminess >= cfg.spam_overall_limit)
                           snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);
                        else
                           snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);

                        link(sdata.ttmpfile, qpath);
                        chmod(qpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                     }


                     syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, spaminess, sdata.tot_len, tvdiff(tv_spam_stop, tv_spam_start)/1000);

                     if(spaminess >= cfg.spam_overall_limit){
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


                        log_ham_spam_per_email(sdata.ttmpfile, email, 1);
                     }
                     else {
                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s", cfg.clapf_header_field, spaminess, cfg.clapf_header_field, sdata.ttmpfile, trainbuf);

                        log_ham_spam_per_email(sdata.ttmpfile, email, 0);
                     }

                  } /* end of running spam check */


                  /* then inject message back */

                  if(spaminess >= cfg.spam_overall_limit){
                    /* shall we redirect the message into oblivion? 2007.02.07, SJ */
                    if(spaminess >= cfg.spaminess_oblivion_limit)
                       inj = ERR_DROP_SPAM;
                    else
                       inj = inject_mail(sdata, i, cfg.spam_smtp_addr, cfg.spam_smtp_port, spaminessbuf, cfg, NULL);
                  }
                  else
                     inj = inject_mail(sdata, i, cfg.postfix_addr, cfg.postfix_port, spaminessbuf, cfg, NULL);

               #else
                  inj = inject_mail(sdata, i, cfg.postfix_addr, cfg.postfix_port, NULL, cfg, NULL);
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

               } /* for */


               /* close database backend handler */

            #ifdef HAVE_ANTISPAM
               free_and_print_list(sstate.first, 0);
            #endif

            #ifdef HAVE_MYSQL
               mysql_close(&mysql);
            #endif
            #ifdef HAVE_SQLITE3
               sqlite3_close(db);
            #endif


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
      snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_421_ERR, cfg.hostid);
      send(new_sd, buf, strlen(buf), 0);
      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

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
