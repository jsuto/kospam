/*
 * session.c, 2008.01.23, SJ
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
#include "black.h"
#include "config.h"

int sd, fd, inj, ret, rav, prevlen=0;
char prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1];
struct timezone tz;
struct timeval tv_start, tv_rcvd, tv_scnd, tv_sent, tv_stop, tv_meta1, tv_meta2;
struct session_data sdata;
int x, rc, unknown_client = 0;

int inject_mail(struct session_data sdata, int msg, char *smtpaddr, int smtpport, char *spaminessbuf, struct __config cfg, char *notify);

#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL mysql;
   int mysql_connection=0;
   int update_mysql_tokens(MYSQL mysql, struct _token *token, unsigned long uid);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   int update_sqlite3_tokens(sqlite3 *db, struct _token *token);
#endif

#ifdef HAVE_STORE
   #include "clapfstore.h"
   store *SSTORE;
#endif

/*
 * kill child if it works too long or is frozen
 */

void kill_child(){
   syslog(LOG_PRIORITY, "%s: child is killed by force", sdata.ttmpfile);
   if(unlink(sdata.ttmpfile)) syslog(LOG_PRIORITY, "%s: failed to remove", sdata.ttmpfile);
   exit(0);
}


/*
 * init SMTP session
 */

void init_child(int new_sd, char *hostid){
   int i;
   char buf[SMALLBUFSIZE];

   memset(sdata.mailfrom, 0, MAXBUFSIZE);
   memset(sdata.client_addr, 0, IPLEN);

   memset(last2buf, 0, 2*MAXBUFSIZE+1);
   memset(prevbuf, 0, MAXBUFSIZE);

   inj = ERR_REJECT;

   sdata.uid = 0;
   sdata.tot_len = 0;
   sdata.skip_id_check = 0;
   prevlen = 0;
   sdata.num_of_rcpt_to = 0;
   unknown_client = 0;

   for(i=0; i<MAX_RCPT_TO; i++){
      memset(sdata.rcptto[i], 0, MAXBUFSIZE);
   }

   gettimeofday(&tv_start, &tz);

   make_rnd_string(&(sdata.ttmpfile[0]));
   unlink(sdata.ttmpfile);

   fd = open(sdata.ttmpfile, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
   if(fd == -1){
       syslog(LOG_PRIORITY, "%s: %s", ERR_OPEN_TMP_FILE, sdata.ttmpfile);
       snprintf(buf, SMALLBUFSIZE-1, SMTP_RESP_421_ERR_TMP, hostid);
       send(new_sd, buf, strlen(buf), 0);
       _exit(0);
   }

}

#ifdef HAVE_LIBCLAMAV
   void postfix_to_clapf(int new_sd, struct __config cfg, struct cl_limits limits, struct cl_node *root){
#else
   void postfix_to_clapf(int new_sd, struct __config cfg){
#endif

   int i, n, state;
   char *p, buf[MAXBUFSIZE], acceptbuf[MAXBUFSIZE], queuedfile[SMALLBUFSIZE], email[SMALLBUFSIZE], email2[SMALLBUFSIZE];

   #ifdef HAVE_ANTIVIRUS
      char virusinfo[SMALLBUFSIZE];
   #endif

   #ifdef HAVE_LIBCLAMAV
      const char *virname;
      unsigned int options=0;
   #endif

   #ifdef HAVE_ANTISPAM
      struct c_res result;
      char spamfile[MAXBUFSIZE], spaminessbuf[MAXBUFSIZE], reason[SMALLBUFSIZE], qpath[SMALLBUFSIZE], trainbuf[SMALLBUFSIZE], ID[RND_STR_LEN+1];
      struct stat st;
      struct timeval tv_spam_start, tv_spam_stop;
      struct _state sstate;
      struct ue UE;
      int is_spam, train_mode=T_TOE;

      result.spaminess=DEFAULT_SPAMICITY;

   #endif

   #ifdef HAVE_AVG
      struct rfc822_attachment Qmime;
      char mimefile[SMALLBUFSIZE];
   #endif

   alarm(cfg.session_timeout);
   signal(SIGALRM, kill_child);

   state = SMTP_STATE_INIT;

   init_child(new_sd, cfg.hostid);


   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: fork()", sdata.ttmpfile);


   // send 220 LMTP banner

#ifdef HAVE_LMTP
   snprintf(buf, MAXBUFSIZE-1, LMTP_RESP_220_BANNER, cfg.hostid);
#else
   snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_220_BANNER, cfg.hostid);
#endif

   send(new_sd, buf, strlen(buf), 0);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

   while((n = recvtimeout(new_sd, buf, MAXBUFSIZE, 0)) > 0){

         // HELO/EHLO

         if(strncasecmp(buf, SMTP_CMD_HELO, strlen(SMTP_CMD_HELO)) == 0 || strncasecmp(buf, SMTP_CMD_EHLO, strlen(SMTP_CMD_EHLO)) == 0 || strncasecmp(buf, LMTP_CMD_LHLO, strlen(LMTP_CMD_LHLO)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            if(state == SMTP_STATE_INIT) state = SMTP_STATE_HELO;

            snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_250_EXTENSIONS, cfg.hostid);
            send(new_sd, buf, strlen(buf), 0);

            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

            /* FIXME: implement the ENHANCEDSTATUSCODE and the PIPELINING extensions */



            continue;
         }

         // XFORWARD

         if(strncasecmp(buf, SMTP_CMD_XFORWARD, strlen(SMTP_CMD_XFORWARD)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            /* extract client address */

            trim(buf);
            p = strstr(buf, "ADDR=");
            if(p){
               snprintf(sdata.client_addr, IPLEN-1, p+5);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: client address: %s", sdata.ttmpfile, sdata.client_addr);
            }

            /*
             * other possible XFORWARD variables
             *
             * XFORWARD NAME=83-131-14-231.adsl.net.t-com.hr ADDR=83.131.14.231..
             * XFORWARD PROTO=SMTP HELO=rhwfsvji..
             */

            /* note if the client is unknown, 2007.12.06, SJ */

            if(strstr(buf, " NAME=unknown ")){
               unknown_client = 1;
            }

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

               /* check against DHA trap address list, 2007.11.06, SJ */

            #ifdef HAVE_BLACKHOLE
               if(strlen(cfg.dha_trap_address_list) > 4){
                  if(extract_email(buf, email) == 1){
                     if(strstr(cfg.dha_trap_address_list, email)){
                        syslog(LOG_PRIORITY, "%s: %s trapped with %s on my DHA list", sdata.ttmpfile, sdata.client_addr, email);
                        put_ip_to_dir(cfg.blackhole_path, sdata.client_addr);
                     }
                  }
               }
            #endif

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

            /* we must send a 250 Ok */

            send(new_sd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);

            /* remove old queue file, 2007.07.17, SJ */

            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);
            unlink(sdata.ttmpfile);

            init_child(new_sd, cfg.hostid);

            state = SMTP_STATE_HELO;

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

               /* make sure we had a successful read, 2007.11.05, SJ */

               rc = fsync(fd);
               close(fd);

               if(rc){
                  syslog(LOG_PRIORITY, "failed writing data: %s", sdata.ttmpfile);

               #ifdef HAVE_LMTP
                  for(i=0; i<sdata.num_of_rcpt_to; i++){
               #endif

                     send(new_sd, SMTP_RESP_421_ERR_WRITE_FAILED, strlen(SMTP_RESP_421_ERR_WRITE_FAILED), 0);

               #ifdef HAVE_LMTP
                  }
               #endif

                  goto AFTER_PERIOD;
               }


         #ifdef HAVE_ANTIVIRUS

               /* antivirus result is ok by default, 2006.02.08, SJ */
               rav = AVIR_OK;

               gettimeofday(&tv_rcvd, &tz);

            #ifdef HAVE_LIBCLAMAV

               options = CL_SCAN_STDOPT | CL_SCAN_ARCHIVE | CL_SCAN_MAIL | CL_SCAN_OLE2;

               /* whether to mark archives as viruses if maxfiles, maxfilesize, or maxreclevel limit is reached, 2006.02.16, SJ */
               if(cfg.use_libclamav_block_max_feature == 1) options |= CL_SCAN_BLOCKMAX;

               /* whether to mark encrypted archives as viruses */
               if(cfg.clamav_block_encrypted_archives == 1) options |= CL_SCAN_BLOCKENCRYPTED;

               /* whether to enable phishing stuff */
               if(cfg.clamav_use_phishing_db == 1) options |= CL_SCAN_PHISHING_DOMAINLIST;

               ret = cl_scanfile(sdata.ttmpfile, &virname, NULL, root, &limits, options);

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

               if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: virus scanning done in %ld [ms]", sdata.ttmpfile, tvdiff(tv_scnd, tv_rcvd)/1000);

               if(rav == AVIR_VIRUS){
                  syslog(LOG_PRIORITY, "%s: Virus found %s", sdata.ttmpfile, virusinfo);

                  /* move to quarantine, if we have to */

                  if(strlen(cfg.quarantine_dir) > 3)
                     move_message_to_quarantine(sdata.ttmpfile, cfg.quarantine_dir, sdata.mailfrom, sdata.rcptto, sdata.num_of_rcpt_to);

                  /* send notification if cfg.localpostmaster is set, 2005.10.04, SJ */

                  if(strlen(cfg.clapfemail) > 3 && strlen(cfg.localpostmaster) > 3){

                     snprintf(buf, MAXBUFSIZE-1, "From: <%s>\r\nTo: <%s>\r\nSubject: %s has been infected\r\n\r\n"
                                    "E-mail from %s to %s (id: %s) was infected\r\n\r\n.\r\n",
                                     cfg.clapfemail, cfg.localpostmaster, sdata.ttmpfile, sdata.mailfrom, sdata.rcptto[0], sdata.ttmpfile);

                     snprintf(sdata.rcptto[0], MAXBUFSIZE-1, "RCPT TO: <%s>\r\n", cfg.localpostmaster);
                     sdata.num_of_rcpt_to = 1;
                     ret = inject_mail(sdata, 0, cfg.postfix_addr, cfg.postfix_port, NULL, cfg, buf);

                     if(ret == 0)
                        syslog(LOG_PRIORITY, "notification about %s to %s failed", sdata.ttmpfile, cfg.localpostmaster);
                  }

               }

         #endif /* HAVE_ANTIVIRUS */


               /* parse the message only once, 2007.10.14, SJ */

         #ifdef HAVE_ANTISPAM
               sstate = parse_message(sdata.ttmpfile, cfg);
               if(unknown_client == 1) sstate.unknown_client = 1;
         #endif


               /* send results back to the '.' command */

               memset(email2, 0, SMALLBUFSIZE);
               extract_email(sdata.mailfrom, email2);

            #ifdef HAVE_LMTP
               for(i=0; i<sdata.num_of_rcpt_to; i++){
            #else
               i = 0;
            #endif
                  if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: round %d in injection", sdata.ttmpfile, i);

                  memset(acceptbuf, 0, MAXBUFSIZE);
                  memset(email, 0, SMALLBUFSIZE);

                  extract_email(sdata.rcptto[i], email);

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
                     memset(trainbuf, 0, SMALLBUFSIZE);
                     memset(spamfile, 0, MAXBUFSIZE);
                     snprintf(spamfile, MAXBUFSIZE-1, "%s/%s", cfg.workdir, sdata.ttmpfile);

                     gettimeofday(&tv_spam_start, &tz);

                     /* skip antispam stuff, if this mail was sent to the blackhole */

                     if(strlen(cfg.blackhole_email_list) > 4){
                        if(strstr(cfg.blackhole_email_list, email)){
                           if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: blackhole mail, skipping spam test", sdata.ttmpfile);
                           goto END_OF_SPAM_CHECK;
                        } 
                     }

                     /* open database backend handler */

                  #ifdef HAVE_MYSQL
                     mysql_init(&mysql);
                     mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
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


                  #ifdef HAVE_MYSQL
                     if(mysql_connection == 1){
                        UE = get_user_from_email(mysql, email);
                        sdata.uid = UE.uid;

                        /* if we have forwarded something for retraining */

                        if(sdata.num_of_rcpt_to == 1 && (str_case_str(sdata.rcptto[0], "+spam@") || str_case_str(sdata.rcptto[0], "+ham@")) ){
                           is_spam = 0;
                           snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                           if(str_case_str(sdata.rcptto[0], "+spam@")) is_spam = 1;

                           UE = get_user_from_email(mysql, email2);
                           sdata.uid = UE.uid;
                           snprintf(sdata.name, SMALLBUFSIZE-1, "%s", UE.name);

                           if(is_spam == 1)
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);
                           else
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);

                           train_mode = extract_id_from_message(sdata.ttmpfile, cfg.clapf_header_field, ID);

                           train_message(mysql, sdata, sstate, MAX_ITERATIVE_TRAIN_LOOPS, is_spam, train_mode, cfg);

                           goto SEND_RESULT;
                        }
                        else {
                           if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running Bayesian test", sdata.ttmpfile);
                           result = bayes_file(mysql, spamfile, sstate, sdata, cfg);
                           update_mysql_tokens(mysql, sstate.first, sdata.uid);

                           if(
                               (cfg.training_mode == T_TUM && ( (result.spaminess >= cfg.spam_overall_limit && result.spaminess < 0.99) || (result.spaminess < cfg.max_ham_spamicity && result.spaminess > 0.1) )) ||
                               (cfg.initial_1000_learning == 1 && (result.ham_msg < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED || result.spam_msg < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED))
                           )
                           {

                              if(result.spaminess >= cfg.spam_overall_limit){
                                 is_spam = 1;
                                 syslog(LOG_PRIORITY, "%s: TUM training a spam", sdata.ttmpfile);
                              }
                              else {
                                 is_spam = 0;
                                 syslog(LOG_PRIORITY, "%s: TUM training a ham", sdata.ttmpfile);
                              }

                              snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg.clapf_header_field);

                              train_message(mysql, sdata, sstate, 1, is_spam, train_mode, cfg);
                           }


                        }

                        gettimeofday(&tv_spam_stop, &tz);
                     }
                     else {
                        gettimeofday(&tv_spam_stop, &tz);
                        result.spaminess = DEFAULT_SPAMICITY;
                     }
                  #endif
                  #ifdef HAVE_SQLITE3
                     if(rc){
                        result.spaminess = DEFAULT_SPAMICITY;
                        gettimeofday(&tv_spam_stop, &tz);
                     }
                     else {
                        rc = sqlite3_exec(db, cfg.sqlite3_pragma, 0, 0, NULL);
                        if(rc != SQLITE_OK) syslog(LOG_PRIORITY, "%s: could not set pragma", sdata.ttmpfile);

                        UE = get_user_from_email(db, email);
                        //sdata.uid = UE.uid;
                        sdata.uid = 0;
                        snprintf(sdata.name, SMALLBUFSIZE-1, "%s", UE.name);

                        /* if we have forwarded something for retraining */

                        if(sdata.num_of_rcpt_to == 1 && (str_case_str(sdata.rcptto[0], "+spam@") || str_case_str(sdata.rcptto[0], "+ham@")) ){
                           is_spam = 0;
                           snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                           if(str_case_str(sdata.rcptto[0], "+spam@")) is_spam = 1;

                           UE = get_user_from_email(db, email2);
                           sdata.uid = UE.uid;

                           if(is_spam == 1)
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);
                           else
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);

                           train_mode = extract_id_from_message(sdata.ttmpfile, cfg.clapf_header_field, ID);

                           train_message(db, sdata, sstate, MAX_ITERATIVE_TRAIN_LOOPS, is_spam, train_mode, cfg);

                           goto SEND_RESULT;
                        }
                        else {
                           if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running Bayesian test", sdata.ttmpfile);
                           result = bayes_file(db, spamfile, sstate, sdata, cfg);
                           update_sqlite3_tokens(db, sstate.first);

                           if(
                               (cfg.training_mode == T_TUM && ( (result.spaminess >= cfg.spam_overall_limit && result.spaminess < 0.99) || (result.spaminess < cfg.max_ham_spamicity && result.spaminess > 0.1) )) ||
                               (cfg.initial_1000_learning == 1 && (result.ham_msg < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED || result.spam_msg < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED))
                           )
                           {

                              if(result.spaminess >= cfg.spam_overall_limit){
                                 is_spam = 1;
                                 syslog(LOG_PRIORITY, "%s: TUM training a spam", sdata.ttmpfile);
                              }
                              else {
                                 is_spam = 0;
                                 syslog(LOG_PRIORITY, "%s: TUM training a ham", sdata.ttmpfile);
                              }

                              snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg.clapf_header_field);

                              train_message(db, sdata, sstate, 1, is_spam, train_mode, cfg);
                           }


                        }

                        gettimeofday(&tv_spam_stop, &tz);
                     }
                  #endif

                  #ifndef OUTGOING_SMTP

                     /* rename file name according to its spamicity status, 2007.10.04, SJ */

                     if(cfg.store_metadata == 1 && strlen(UE.name) > 1){
                        if(result.spaminess >= cfg.spam_overall_limit)
                           snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);
                        else
                           snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);

                     #ifdef HAVE_STORE
                        gettimeofday(&tv_meta1, &tz);

                        SSTORE = store_init(cfg.store_addr, cfg.store_port);
                        if(SSTORE){
                          if(SSTORE->rc == 1){
                           if(result.spaminess >= cfg.spam_overall_limit)
                              rc = store_email(SSTORE, sdata.ttmpfile, UE.name, sdata.ttmpfile, 1, cfg.store_secret);
                           else
                              rc = store_email(SSTORE, sdata.ttmpfile, UE.name, sdata.ttmpfile, 0, cfg.store_secret);
                          }

                          store_free(SSTORE);
                        }

                        gettimeofday(&tv_meta2, &tz);


                     #else
                        if(cfg.store_only_spam == 1 && result.spaminess < cfg.spam_overall_limit){
                        }
                        else {
                           link(sdata.ttmpfile, qpath);
                           if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: try to link to %s", sdata.ttmpfile, qpath);

                           if(stat(qpath, &st) == 0){
                              if(S_ISREG(st.st_mode) == 1)
                                 chmod(qpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                           }

                        }
                     #endif
                     }

                  #endif

                     syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, result.spaminess, sdata.tot_len, tvdiff(tv_spam_stop, tv_spam_start)/1000);

                     if(result.spaminess >= cfg.spam_overall_limit){
                        memset(reason, 0, SMALLBUFSIZE);

                        if(result.spaminess == cfg.spaminess_of_strange_language_stuff) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_STRANGE_LANGUAGE);
                        if(result.spaminess == cfg.spaminess_of_too_much_spam_in_top15) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_TOO_MUCH_SPAM_IN_TOP15);
                        if(result.spaminess == cfg.spaminess_of_blackholed_mail) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_BLACKHOLED);
                        if(result.spaminess == cfg.spaminess_of_caught_by_surbl) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_CAUGHT_BY_SURBL);
                        if(result.spaminess == cfg.spaminess_of_text_and_base64) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_TEXT_AND_BASE64);
                        if(result.spaminess == cfg.spaminess_of_embed_image) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_EMBED_IMAGE);
                        if(result.spaminess > 0.9999) snprintf(reason, SMALLBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, MSG_ABSOLUTELY_SPAM);

                        /* add additional headers, credits: Mariano, 2006.08.14 */

                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s%s%s\r\n",
                           cfg.clapf_header_field, result.spaminess, cfg.clapf_header_field, sdata.ttmpfile, reason, trainbuf, cfg.clapf_spam_header_field);


                        log_ham_spam_per_email(sdata.ttmpfile, email, 1);
                     }
                     else {
                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s", cfg.clapf_header_field, result.spaminess, cfg.clapf_header_field, sdata.ttmpfile, trainbuf);

                        log_ham_spam_per_email(sdata.ttmpfile, email, 0);
                     }

                  } /* end of running spam check */


               END_OF_SPAM_CHECK:

                  /* then inject message back */

                  if(result.spaminess >= cfg.spam_overall_limit){
                    /* shall we redirect the message into oblivion? 2007.02.07, SJ */
                    if(result.spaminess >= cfg.spaminess_oblivion_limit)
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

                  if(inj == ERR_DROP_SPAM) syslog(LOG_PRIORITY, "%s: dropped spam", sdata.ttmpfile);
                  else if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, acceptbuf);

            #ifdef HAVE_LMTP
               } /* for */
            #endif


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

         AFTER_PERIOD:

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
      write_delivery_info(sdata.ttmpfile, cfg.queuedir, sdata.mailfrom, sdata.rcptto, sdata.num_of_rcpt_to);
      link(sdata.ttmpfile, queuedfile);
   }

   if(unlink(sdata.ttmpfile)) syslog(LOG_PRIORITY, "%s: failed to remove", sdata.ttmpfile);
   if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);

   _exit(0);

}
