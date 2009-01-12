/*
 * session.c, 2009.01.12, SJ
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

   memset(sdata->mailfrom, 0, MAXBUFSIZE);
   memset(sdata->client_addr, 0, IPLEN);

   sdata->uid = 0;
   sdata->tot_len = 0;
   sdata->skip_id_check = 0;
   sdata->num_of_rcpt_to = 0;
   sdata->unknown_client = 0;
   sdata->Nham = 0;
   sdata->Nspam = 0;

   for(i=0; i<MAX_RCPT_TO; i++){
      memset(sdata->rcptto[i], 0, MAXBUFSIZE);
   }

}


#ifdef HAVE_LIBCLAMAV
   void postfix_to_clapf(int new_sd, struct __config cfg, struct cl_limits limits, struct cl_node *root){
#else
   void postfix_to_clapf(int new_sd, struct __config cfg){
#endif

   int i, n, rav=AVIR_OK, inj=ERR_REJECT, state, prevlen=0;
   char *p, buf[MAXBUFSIZE], prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1], acceptbuf[MAXBUFSIZE];
   char email[SMALLBUFSIZE], email2[SMALLBUFSIZE];
   float spaminess;
   struct session_data sdata;
   struct __config my_cfg;
   struct timezone tz;
   struct timeval tv_rcvd, tv_scnd;
   int is_spam;
   int rc;
   struct ue UE, UE2;

   #ifdef HAVE_ANTIVIRUS
      int ret;
      char virusinfo[SMALLBUFSIZE], engine[SMALLBUFSIZE];
   #endif

   #ifdef HAVE_LIBCLAMAV
      const char *virname;
      unsigned int options=0;
   #endif

#ifdef NEED_MYSQL
   MYSQL mysql;
   int mysql_connection=0;
#endif
#ifdef NEED_SQLITE3
   sqlite3 *db;
#endif
#ifdef NEED_LDAP
   LDAP *ldap;
#endif

   #ifdef HAVE_ANTISPAM
      char spaminessbuf[MAXBUFSIZE], reason[SMALLBUFSIZE], qpath[SMALLBUFSIZE], trainbuf[SMALLBUFSIZE], whitelistbuf[SMALLBUFSIZE], ID[RND_STR_LEN+1];
      struct stat st;
      struct timeval tv_spam_start, tv_spam_stop;
      struct _state sstate, sstate2;
      int train_mode=T_TOE;

      spaminess=DEFAULT_SPAMICITY;

   #endif

   #ifdef HAVE_AVG
      struct rfc822_attachment Qmime;
      char mimefile[SMALLBUFSIZE];
   #endif

   alarm(cfg.session_timeout);
   signal(SIGALRM, kill_child);

   state = SMTP_STATE_INIT;

   init_session_data(&sdata);

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: fork()", sdata.ttmpfile);

   /* send 220 SMTP/LMTP banner */

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
               sdata.unknown_client = 1;
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

               memset(email2, 0, SMALLBUFSIZE);
               extract_email(sdata.mailfrom, email2);
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


            memset(last2buf, 0, 2*MAXBUFSIZE+1);
            memset(prevbuf, 0, MAXBUFSIZE);
            inj = ERR_REJECT;
            prevlen = 0;

            if(state != SMTP_STATE_RCPT_TO){
               send(new_sd, SMTP_RESP_503_ERR, strlen(SMTP_RESP_503_ERR), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_503_ERR);
            }
            else {
               sdata.fd = open(sdata.ttmpfile, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
               if(sdata.fd == -1){
                  syslog(LOG_PRIORITY, "%s: %s", ERR_OPEN_TMP_FILE, sdata.ttmpfile);
                  send(new_sd, SMTP_RESP_451_ERR, strlen(SMTP_RESP_451_ERR), 0);
               }
               else {
                  state = SMTP_STATE_DATA;
                  send(new_sd, SMTP_RESP_354_DATA_OK, strlen(SMTP_RESP_354_DATA_OK), 0);
                  if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_354_DATA_OK);
               }

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

            unlink(sdata.ttmpfile);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);

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

            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);
            unlink(sdata.ttmpfile);

            init_session_data(&sdata);

            state = SMTP_STATE_HELO;

            continue;
         }


         /* accept mail data */

         if(state == SMTP_STATE_DATA){
            write(sdata.fd, buf, n);
            sdata.tot_len += n;

            /* join the last 2 buffer, 2004.08.30, SJ */

            memset(last2buf, 0, 2*MAXBUFSIZE+1);
            memcpy(last2buf, prevbuf, MAXBUFSIZE);
            memcpy(last2buf+prevlen, buf, MAXBUFSIZE);

            if(search_in_buf(last2buf, 2*MAXBUFSIZE+1, SMTP_CMD_PERIOD, 5) == 1){

               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: (.)", sdata.ttmpfile);

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

                  goto AFTER_PERIOD;
               }

               write_delivery_info(&sdata, cfg.workdir);

         #ifdef HAVE_ANTIVIRUS

               /* antivirus result is ok by default, 2006.02.08, SJ */
               rav = AVIR_OK;
               memset(engine, 0, SMALLBUFSIZE);

               gettimeofday(&tv_rcvd, &tz);

            #ifdef HAVE_LIBCLAMAV
               options = CL_SCAN_STDOPT | CL_SCAN_ARCHIVE | CL_SCAN_MAIL | CL_SCAN_OLE2;

               /* whether to mark archives as viruses if maxfiles, maxfilesize, or maxreclevel limit is reached, 2006.02.16, SJ */
               if(cfg.use_libclamav_block_max_feature == 1) options |= CL_SCAN_BLOCKMAX;

               /* whether to mark encrypted archives as viruses */
               if(cfg.clamav_block_encrypted_archives == 1) options |= CL_SCAN_BLOCKENCRYPTED;

               /* whether to enable phishing stuff */
               //if(cfg.clamav_use_phishing_db == 1) options |= CL_SCAN_PHISHING_DOMAINLIST;

               ret = cl_scanfile(sdata.ttmpfile, &virname, NULL, root, &limits, options);

               if(ret == CL_VIRUS){
                  memset(virusinfo, 0, SMALLBUFSIZE);
                  strncpy(virusinfo, virname, SMALLBUFSIZE-1);
                  rav = AVIR_VIRUS;
                  snprintf(engine, SMALLBUFSIZE-1, "libClamAV");
               }
            #endif
            #ifdef HAVE_AVG
               /* extract attachments from message file */

               Qmime = extract_from_rfc822(sdata.ttmpfile);
               if(Qmime.result == 1){

                  if(Qmime.cnt > 0){

                     /* scan directory */

                     if(avg_scan(Qmime.tmpdir, sdata.ttmpfile, engine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;

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
               if(avast_scan(sdata.ttmpfile, engine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;
            #endif
            #ifdef HAVE_KAV
               if(kav_scan(sdata.ttmpfile, engine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;
            #endif
            #ifdef HAVE_DRWEB
               if(drweb_scan(sdata.ttmpfile, engine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;
            #endif
            #ifdef HAVE_CLAMD
               if(strlen(cfg.clamd_addr) > 3){
                  if(clamd_net_scan(sdata.ttmpfile, engine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;
               } else {
                  if(clamd_scan(sdata.ttmpfile, engine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;
               }
            #endif

               gettimeofday(&tv_scnd, &tz);

               if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: virus scanning done in %ld [ms]", sdata.ttmpfile, tvdiff(tv_scnd, tv_rcvd)/1000);

               if(rav == AVIR_VIRUS){
                  syslog(LOG_PRIORITY, "%s: Virus found %s", sdata.ttmpfile, virusinfo);

                  /* move to quarantine, if we have to */

                  if(strlen(cfg.quarantine_dir) > 3)
                     move_message_to_quarantine(&sdata, cfg.quarantine_dir);

                  /* send notification if cfg.localpostmaster is set, 2005.10.04, SJ */

                  if(strlen(cfg.localpostmaster) > 3){

                     memset(email, 0, SMALLBUFSIZE);
                     extract_email(sdata.rcptto[0], email);

                     if(get_template(VIRUS_TEMPLATE, buf, cfg.localpostmaster, email, email2, virusinfo, engine) == 1){

                        snprintf(sdata.rcptto[0], MAXBUFSIZE-1, "RCPT TO: <%s>\r\n", cfg.localpostmaster);
                        sdata.num_of_rcpt_to = 1;
                        ret = inject_mail(&sdata, 0, cfg.postfix_addr, cfg.postfix_port, NULL, &cfg, buf);

                        if(ret == 0)
                           syslog(LOG_PRIORITY, "notification about %s to %s failed", sdata.ttmpfile, cfg.localpostmaster);
                     }
                  }

               }

         #endif /* HAVE_ANTIVIRUS */


               /* parse the message only once, 2007.10.14, SJ */

         #ifdef HAVE_ANTISPAM

               if((strstr(sdata.mailfrom, "MAILER-DAEMON") || strstr(sdata.mailfrom, "<>")) && strlen(cfg.our_signo) > 3){
                  if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "from: %s, we should really see our signo", sdata.mailfrom);
                  sdata.need_signo_check = 1;
               }

               memset(whitelistbuf, 0, SMALLBUFSIZE);
               sstate = parse_message(sdata.ttmpfile, &sdata, &cfg);

               if(sdata.need_signo_check == 1){
                  if(sstate.found_our_signo == 1)
                     syslog(LOG_PRIORITY, "%s: found our signo, this should be a real bounce message", sdata.ttmpfile);
                  else
                     syslog(LOG_PRIORITY, "%s: looks like a bounce, but our signo is missing", sdata.ttmpfile);
               }

         #endif



               /* open database backend handler */

            #ifdef NEED_MYSQL
               mysql_init(&mysql);
               mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
               if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
                  mysql_connection = 1;
               else {
                  mysql_connection = 0;
                  syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_MYSQL_CONNECT);
               }
            #endif
            #ifdef NEED_SQLITE3
               rc = sqlite3_open(cfg.sqlite3, &db);
               if(rc){
                  syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_SQLITE3_OPEN);
               }
            #endif


            /* get user from 'MAIL FROM:', 2008.10.25, SJ */

            #ifdef USERS_IN_MYSQL
               UE2 = get_user_from_email(mysql, email2);
            #endif
            #ifdef USERS_IN_SQLITE3
               UE2 = get_user_from_email(db, email2);
            #endif
            #ifdef USERS_IN_LDAP
               ldap = do_bind_ldap(cfg.ldap_host, cfg.ldap_user, cfg.ldap_pwd, cfg.ldap_use_tls);
               UE2 = get_user_from_email(ldap, email2, &cfg);
            #endif

               /* copy default config from clapf.conf, to enable policy support later, 2008.11.26, SJ */

               my_cfg = cfg;

            #ifdef HAVE_LMTP
               for(i=0; i<sdata.num_of_rcpt_to; i++){
            #else
               i = 0;
            #endif
                  if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: round %d in injection", sdata.ttmpfile, i);

                  memset(acceptbuf, 0, MAXBUFSIZE);
                  memset(email, 0, SMALLBUFSIZE);

                  is_spam = 0;
                  spaminess = DEFAULT_SPAMICITY;
                  UE.policy_group = 0;

                  extract_email(sdata.rcptto[i], email);


                  /* get user from 'RCPT TO:', 2008.11.24, SJ */

               #ifdef USERS_IN_MYSQL
                  UE = get_user_from_email(mysql, email);
               #endif
               #ifdef USERS_IN_SQLITE3
                  UE = get_user_from_email(db, email);
               #endif
               #ifdef USERS_IN_LDAP
                  UE = get_user_from_email(ldap, email, &cfg);
               #endif

                  /* read policy, 2008.11.24, SJ */

               #ifdef HAVE_POLICY
                  #ifdef USERS_IN_MYSQL
                     if(UE.policy_group > 0) get_policy(mysql, &cfg, &my_cfg, UE.policy_group, sdata.num_of_rcpt_to);
                  #endif

                  #ifdef USERS_IN_LDAP
                     if(UE.policy_group > 0) get_policy(ldap, cfg.ldap_base, &cfg, &my_cfg, UE.policy_group, sdata.num_of_rcpt_to);
                  #endif
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

                  /* run statistical antispam check */

                  if(my_cfg.use_antispam == 1 && (my_cfg.max_message_size_to_filter == 0 || sdata.tot_len < my_cfg.max_message_size_to_filter) ){
                     memset(trainbuf, 0, SMALLBUFSIZE);

                     gettimeofday(&tv_spam_start, &tz);

                     /* skip antispam stuff, if this mail was sent to the blackhole */

                     if(strlen(cfg.blackhole_email_list) > 4){
                        if(strstr(cfg.blackhole_email_list, email)){
                           if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: blackhole mail, skipping spam test", sdata.ttmpfile);
                           goto END_OF_SPAM_CHECK;
                        } 
                     }


                  /*#ifdef HAVE_MYDB
                     if(sdata.num_of_rcpt_to == 1 && (strcasestr(sdata.rcptto[0], "+spam@") || strcasestr(sdata.rcptto[0], "+ham@") || strncmp(email, "spam@", 5) == 0 || strncmp(email, "ham@", 4) == 0) ){
                        is_spam = 0;
                        snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                        if(strcasestr(sdata.rcptto[0], "+spam@") || strncmp(email, "spam@", 5) == 0) is_spam = 1;

                        sdata.uid = 0;
                        snprintf(sdata.name, SMALLBUFSIZE-1, "%s", UE2.name);

                        train_mode = extract_id_from_message(sdata.ttmpfile, cfg.clapf_header_field, ID);
                        syslog(LOG_PRIORITY, "%s: training request for %s by uid: %ld", sdata.ttmpfile, ID, UE2.uid);

                        sstate2.first = NULL; // just to get rid of the compiler warning

                        goto SEND_RESULT;
                     }
                     else {

                        // TODO: check for whitelist

                        spaminess = x_spam_check(&sdata, &sstate, cfg);

                        gettimeofday(&tv_spam_stop, &tz);
                     }
                  #endif*/

                  #ifdef HAVE_MYSQL
                     if(mysql_connection == 1){
                        sdata.uid = UE.uid;

                        /* if we have forwarded something for retraining */

                        if(sdata.num_of_rcpt_to == 1 && (strcasestr(sdata.rcptto[0], "+spam@") || strcasestr(sdata.rcptto[0], "+ham@") || strncmp(email, "spam@", 5) == 0 || strncmp(email, "ham@", 4) == 0 ) ){
                           is_spam = 0;
                           snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                           if(strcasestr(sdata.rcptto[0], "+spam@") || strncmp(email, "spam@", 5) == 0) is_spam = 1;

                           sdata.uid = UE2.uid;
                           snprintf(sdata.name, SMALLBUFSIZE-1, "%s", UE2.name);

                           train_mode = extract_id_from_message(sdata.ttmpfile, cfg.clapf_header_field, ID);

                           syslog(LOG_PRIORITY, "%s: training request for %s by uid: %ld", sdata.ttmpfile, ID, UE2.uid);

                           if(is_spam == 1)
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, UE2.name[0], UE2.name, ID);
                           else
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, UE2.name[0], UE2.name, ID);

                           sstate2 = parse_message(qpath, &sdata, &my_cfg);

                           train_message(mysql, sdata, sstate2, MAX_ITERATIVE_TRAIN_LOOPS, is_spam, train_mode, my_cfg);

                           free_url_list(sstate2.urls);
                           clearhash(sstate2.token_hash, 0);

                           goto SEND_RESULT;
                        }
                        else {

                           if(is_sender_on_white_list(mysql, email2, sdata.uid, my_cfg)){
                              syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata.ttmpfile, email);
                              snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on white list\r\n", cfg.clapf_header_field);
                              goto END_OF_SPAM_CHECK;
                           }
                           else {
                              if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running Bayesian test", sdata.ttmpfile);
                              spaminess = bayes_file(mysql, &sstate, &sdata, &my_cfg);
                           }
                           update_mysql_tokens(mysql, sstate.token_hash, sdata.uid);

                           if(
                               (my_cfg.training_mode == T_TUM && ( (spaminess >= my_cfg.spam_overall_limit && spaminess < 0.99) || (spaminess < my_cfg.max_ham_spamicity && spaminess > 0.1) )) ||
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

                              snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg.clapf_header_field);

                              train_message(mysql, sdata, sstate, 1, is_spam, train_mode, my_cfg);
                           }


                        }

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
                        rc = sqlite3_exec(db, cfg.sqlite3_pragma, 0, 0, NULL);
                        if(rc != SQLITE_OK) syslog(LOG_PRIORITY, "%s: could not set pragma", sdata.ttmpfile);

                        sdata.uid = 0;
                        snprintf(sdata.name, SMALLBUFSIZE-1, "%s", UE.name);

                        /* if we have forwarded something for retraining */

                        if(sdata.num_of_rcpt_to == 1 && (strcasestr(sdata.rcptto[0], "+spam@") || strcasestr(sdata.rcptto[0], "+ham@") || strncmp(email, "spam@", 5) == 0 || strncmp(email, "ham@", 4) == 0) ){
                           is_spam = 0;
                           snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, email);
                           if(strcasestr(sdata.rcptto[0], "+spam@") || strncmp(email, "spam@", 5) == 0) is_spam = 1;

                           sdata.uid = UE2.uid;

                           train_mode = extract_id_from_message(sdata.ttmpfile, cfg.clapf_header_field, ID);

                           syslog(LOG_PRIORITY, "%s: training request for %s by uid: %ld", sdata.ttmpfile, ID, UE2.uid);

                           if(is_spam == 1)
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, UE2.name[0], UE2.name, ID);
                           else
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, UE2.name[0], UE2.name, ID);


                           sstate2 = parse_message(qpath, &sdata, &cfg);

                           train_message(db, sdata, sstate2, MAX_ITERATIVE_TRAIN_LOOPS, is_spam, train_mode, cfg);

                           free_url_list(sstate2.urls);
                           clearhash(sstate2.token_hash, 0);


                           goto SEND_RESULT;
                        }
                        else {

                           if(is_sender_on_white_list(db, email2, sdata.uid, cfg)){
                              syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata.ttmpfile, email);
                              snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on white list\r\n", cfg.clapf_header_field);
                              goto END_OF_SPAM_CHECK;
                           }
                           else {
                              if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running Bayesian test", sdata.ttmpfile);
                              spaminess = bayes_file(db, &sstate, &sdata, &cfg);
                           }
                           update_sqlite3_tokens(db, sstate.token_hash);

                           if(
                               (cfg.training_mode == T_TUM && ( (spaminess >= cfg.spam_overall_limit && spaminess < 0.99) || (spaminess < cfg.max_ham_spamicity && spaminess > 0.1) )) ||
                               (cfg.initial_1000_learning == 1 && (sdata.Nham < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED || sdata.Nspam < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED))
                           )
                           {

                              if(spaminess >= cfg.spam_overall_limit){
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

                        if(cfg.store_only_spam == 1 && spaminess < cfg.spam_overall_limit){
                        }
                        else {
                           snprintf(qpath, SMALLBUFSIZE-1, "%s/%c", USER_QUEUE_DIR, UE.name[0]);
                           if(stat(qpath, &st))
                              mkdir(qpath, QUEUE_DIR_PERMISSION);

                           snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s", USER_QUEUE_DIR, UE.name[0], UE.name);
                           if(stat(qpath, &st)){
                              mkdir(qpath, QUEUE_DIR_PERMISSION);

                              /*
                               * the web server must have write permissions on the user's queue directory.
                               * you have to either extend these rights the to world, ie. 777 or
                               * change group-id of clapf to the web server, ie. usermod -g www-data clapf.
                               * 2008.10.27, SJ
                               */

                              chmod(qpath, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP);
                           }

                           if(spaminess >= cfg.spam_overall_limit)
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);
                           else
                              snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, UE.name[0], UE.name, sdata.ttmpfile);

                           link(sdata.ttmpfile, qpath);
                           if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: try to link to %s", sdata.ttmpfile, qpath);

                           if(stat(qpath, &st) == 0){
                              if(S_ISREG(st.st_mode) == 1)
                                 chmod(qpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                           }

                        }


                        /* this info is not used yet, 2008.10.29, SJ

                     #ifdef HAVE_MYSQL
                        insert_2_queue(mysql, &sdata, cfg, is_spam);
                     #endif
                     #ifdef HAVE_SQLITE3
                        insert_2_queue(db, &sdata, cfg, is_spam);
                     #endif
                         */

                     }

                  #endif

                     syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, spaminess, sdata.tot_len, tvdiff(tv_spam_stop, tv_spam_start)/1000);


                     if(sdata.need_signo_check == 1){
                        if(!sstate.found_our_signo){
                           syslog(LOG_PRIORITY, "%s: looks like a bounce, but our signo is missing", sdata.ttmpfile);
                           if(spaminess < cfg.spam_overall_limit){
                              spaminess = 0.99;
                              syslog(LOG_PRIORITY, "%s: raising spamicity", sdata.ttmpfile);
                           }
                        }
                        else {
                           syslog(LOG_PRIORITY, "found our signo, this should be a real bounce message");
                           spaminess = DEFAULT_SPAMICITY;
                        }
                     }


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

                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s%ld ms\r\n%s%s%s%s\r\n",
                           cfg.clapf_header_field, spaminess, cfg.clapf_header_field, sdata.ttmpfile, cfg.clapf_header_field, tvdiff(tv_spam_stop, tv_spam_start)/1000,
                           reason, trainbuf, whitelistbuf, cfg.clapf_spam_header_field);


                        log_ham_spam_per_email(sdata.ttmpfile, email, 1);
                     }
                     else {
                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s%ld ms\r\n%s%s",
                            cfg.clapf_header_field, spaminess, cfg.clapf_header_field, sdata.ttmpfile, cfg.clapf_header_field, tvdiff(tv_spam_stop, tv_spam_start)/1000, trainbuf, whitelistbuf);

                        log_ham_spam_per_email(sdata.ttmpfile, email, 0);
                     }

                  } /* end of running spam check */

                  /* set a reasonable clapf header if no spam check has happened, 2008.04.14, SJ */

                  else {
                     snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s%ld ms\r\n",
                              cfg.clapf_header_field, spaminess, cfg.clapf_header_field, sdata.ttmpfile, cfg.clapf_header_field, tvdiff(tv_spam_stop, tv_spam_start)/1000);
                  }


               END_OF_SPAM_CHECK:

                  /* then inject message back */

                  if(spaminess >= cfg.spam_overall_limit){
                    /* shall we redirect the message into oblivion? 2007.02.07, SJ */
                    if(spaminess >= cfg.spaminess_oblivion_limit)
                       inj = ERR_DROP_SPAM;
                    else
                       inj = inject_mail(&sdata, i, cfg.spam_smtp_addr, cfg.spam_smtp_port, spaminessbuf, &my_cfg, NULL);
                  }
                  else
                     inj = inject_mail(&sdata, i, cfg.postfix_addr, cfg.postfix_port, spaminessbuf, &my_cfg, NULL);

               #else
               END_OF_SPAM_CHECK:
                  inj = inject_mail(&sdata, i, cfg.postfix_addr, cfg.postfix_port, NULL, &my_cfg, NULL);
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

               /* 2009.09.02, SJ */
               unlink(sdata.ttmpfile);
               unlink(sdata.deliveryinfo);

               /* close database backend handler */

            #ifdef HAVE_ANTISPAM
               free_url_list(sstate.urls);
               clearhash(sstate.token_hash, 0);
            #endif

            #ifdef NEED_MYSQL
               mysql_close(&mysql);
               mysql_connection = 0;
            #endif
            #ifdef NEED_SQLITE3
               sqlite3_close(db);
               rc = SQLITE_ERROR;
            #endif
            #ifdef NEED_IN_LDAP
               ldap_unbind_s(ldap);
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

   if(cfg.verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "child has finished");

   _exit(0);
}
