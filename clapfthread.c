/*
 * clapf.c, 2008.02.07, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "misc.h"
#include "errmsg.h"
#include "smtpcodes.h"
#include "messages.h"
#include "mime.h"
#include "avg.h"
#include "avast.h"
#include "kav.h"
#include "drweb.h"
#include "clamd.h"
#include "bayes.h"
#include "sql.h"
#include "config.h"

#ifdef HAVE_MYSQL
   #include <mysql.h>
   int update_mysql_tokens(MYSQL mysql, struct _token *token, unsigned long uid);
#endif
#ifdef HAVE_STORE
   #include "clapfstore.h"
#endif

extern char *optarg;
extern int optind;


char *configfile = CONFIG_FILE;
struct __config cfg;

int inject_mail(struct session_data sdata, int msg, char *smtpaddr, int smtpport, char *spaminessbuf, struct __config cfg, char *notify);


typedef struct {
   int sockfd;
   pthread_t thread;
} qconn;


#ifdef HAVE_LIBCLAMAV
   #include <clamav.h>
   struct cl_stat dbstat;
   struct cl_limits limits;
   struct cl_engine *engine = NULL;
   const char *dbdir;
   unsigned int options=0;
#endif

struct timezone tz;
pthread_mutex_t __lock;
pthread_attr_t attr;
int __num_threads, listener;
char too_many_connections[SMALLBUFSIZE];

/*
 * shutdown clapf
 */

void clapf_exit(){
   close(listener);
   pthread_attr_destroy(&attr);

#ifdef HAVE_LIBCLAMAV
   if(engine)
      cl_free(engine);
#endif

   syslog(LOG_PRIORITY, "%s has been terminated", PROGNAME);

   unlink(cfg.pidfile);

   exit(1);
}


/*
 * exit with a message
 */

void fatal(char *s){
   printf("%s\n", s);
   clapf_exit();
}


/*
 * increment thread counter
 */

void increment_thread_count(void){
   pthread_mutex_lock(&__lock);
   __num_threads++;
   pthread_mutex_unlock(&__lock);
}


/*
 * decrement thread counter
 */

void decrement_thread_count(void){
   pthread_mutex_lock(&__lock);
   __num_threads--;
   pthread_mutex_unlock(&__lock);
}


/*
 * reload configuration
 */

void reload_config(){
   cfg = read_config(configfile);

   snprintf(too_many_connections, SMALLBUFSIZE-1, SMTP_RESP_421_ERR_TMP, cfg.hostid);

   if(chdir(cfg.workdir))
      fatal(ERR_CHDIR);

#ifdef HAVE_LIBCLAMAV
    /* set up archive limits */

    memset(&limits, 0, sizeof(struct cl_limits));

    limits.maxfiles = cfg.clamav_maxfile;
    limits.maxfilesize = cfg.clamav_max_archived_file_size;
    limits.maxreclevel = cfg.clamav_max_recursion_level;
    limits.maxratio = cfg.clamav_max_compress_ratio;
    limits.archivememlim = cfg.clamav_archive_mem_limit;

    if(cfg.clamav_use_phishing_db == 1)
       options = CL_DB_STDOPT|CL_DB_PHISHING|CL_DB_PHISHING_URLS;
    else
       options = 0;
#endif

   syslog(LOG_PRIORITY, "reloaded config: %s", configfile);
}


#ifdef HAVE_LIBCLAMAV
void reload_clamav_db(){
   int retval;
   unsigned int sigs = 0;

   /* release old structure */
   if(engine){
      cl_free(engine);
      engine = NULL;
   }

   /* get default database directory */
   dbdir = cl_retdbdir();
   if(dbdir == NULL)
      fatal(ERR_NO_DB_DIR);

   /* initialise dbstat structure */
   memset(&dbstat, 0, sizeof(struct cl_stat));
   if(cl_statinidir(dbdir, &dbstat) != 0)
      fatal(ERR_STAT_INI_DIR);

   /* load virus signatures from database(s) */

   if((retval = cl_load(cl_retdbdir(), &engine, &sigs, options))){
      syslog(LOG_PRIORITY, "reloading db failed: %s", cl_strerror(retval));
      clapf_exit();
   }

   if((retval = cl_build(engine)) != 0){
      syslog(LOG_PRIORITY, "Database initialization error: can't build engine: %s", cl_strerror(retval));
      cl_free(engine);
      clapf_exit();
   }

   syslog(LOG_PRIORITY, "reloaded with %d viruses", sigs);
}
#endif



/*
 * process a connection
 */

void *process_connection(void *ptr){
   qconn *QC = (qconn*)ptr;
   struct session_data sdata;
   char prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1];
   int fd = -1, inj, rav=AVIR_OK, prevlen=0;
   int rc, unknown_client = 0;
   struct timezone tz;
   struct timeval tv_start;
   int i, n, state;
   char *p, buf[MAXBUFSIZE], acceptbuf[MAXBUFSIZE], email[SMALLBUFSIZE], email2[SMALLBUFSIZE];

#ifdef HAVE_ANTIVIRUS
   struct timeval tv_rcvd, tv_scnd;
   char virusinfo[SMALLBUFSIZE];
   int ret;
#endif
#ifdef HAVE_LIBCLAMAV
   const char *virname;
   unsigned int options=0;
#endif

#ifdef HAVE_ANTISPAM
   struct c_res result;
   char spamfile[MAXBUFSIZE], spaminessbuf[MAXBUFSIZE], reason[SMALLBUFSIZE], qpath[SMALLBUFSIZE], trainbuf[SMALLBUFSIZE], whitelistbuf[SMALLBUFSIZE], ID[RND_STR_LEN+1];
   struct stat st;
   struct timeval tv_spam_start, tv_spam_stop;
   struct _state sstate;
   struct ue UE;
   int is_spam, train_mode=T_TOE, db_connection=0;
#endif

#ifdef HAVE_MYSQL
   MYSQL mysql;
#endif
#ifdef HAVE_STORE
   store *SSTORE;
#endif


   state = SMTP_STATE_INIT;


   /* initialise data */

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

   for(i=0; i<MAX_RCPT_TO; i++)
      memset(sdata.rcptto[i], 0, MAXBUFSIZE);

   gettimeofday(&tv_start, &tz);

   make_rnd_string(&(sdata.ttmpfile[0]));
   unlink(sdata.ttmpfile);

   /* end of initialisation */


   /* open database backend handler */

#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      db_connection = 1;
   else {
      db_connection = 0;
      syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_MYSQL_CONNECT);
   }
#endif

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: pthread_create()", sdata.ttmpfile);

   // send 220 LMTP banner

#ifdef HAVE_LMTP
   snprintf(buf, MAXBUFSIZE-1, LMTP_RESP_220_BANNER, cfg.hostid);
#else
   snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_220_BANNER, cfg.hostid);
#endif

   send(QC->sockfd, buf, strlen(buf), 0);
   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

   while((n = recvtimeout(QC->sockfd, buf, MAXBUFSIZE, 0)) > 0){

         // HELO/EHLO

         if(strncasecmp(buf, SMTP_CMD_HELO, strlen(SMTP_CMD_HELO)) == 0 || strncasecmp(buf, SMTP_CMD_EHLO, strlen(SMTP_CMD_EHLO)) == 0 || strncasecmp(buf, LMTP_CMD_LHLO, strlen(LMTP_CMD_LHLO)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            if(state == SMTP_STATE_INIT) state = SMTP_STATE_HELO;

            snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_250_EXTENSIONS, cfg.hostid);
            send(QC->sockfd, buf, strlen(buf), 0);

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

            send(QC->sockfd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);

            continue;            
         }

         // MAIL FROM

         if(strncasecmp(buf, SMTP_CMD_MAIL_FROM, strlen(SMTP_CMD_MAIL_FROM)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            if(state != SMTP_STATE_HELO){
               send(QC->sockfd, SMTP_RESP_503_ERR, strlen(SMTP_RESP_503_ERR), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_503_ERR);
            } 
            else {
               state = SMTP_STATE_MAIL_FROM;
               memcpy(sdata.mailfrom, buf, MAXBUFSIZE-1);
               send(QC->sockfd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
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

               send(QC->sockfd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);
            }
            else {
               send(QC->sockfd, SMTP_RESP_503_ERR, strlen(SMTP_RESP_503_ERR), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_503_ERR);
            }

            continue;
         }

         // DATA

         if(strncasecmp(buf, SMTP_CMD_DATA, strlen(SMTP_CMD_DATA)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            if(state != SMTP_STATE_RCPT_TO){
               send(QC->sockfd, SMTP_RESP_503_ERR, strlen(SMTP_RESP_503_ERR), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_503_ERR);
            }
            else {
               state = SMTP_STATE_DATA;

               /* open tmp file */

              fd = open(sdata.ttmpfile, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
              if(fd == -1){
                 syslog(LOG_PRIORITY, "%s: %s", ERR_OPEN_TMP_FILE, sdata.ttmpfile);
                 send(QC->sockfd, SMTP_RESP_451_ERR, strlen(SMTP_RESP_451_ERR), 0);
                 continue;
               }

               send(QC->sockfd, SMTP_RESP_354_DATA_OK, strlen(SMTP_RESP_354_DATA_OK), 0);
               if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_354_DATA_OK);
            }

            continue;
         }

         // QUIT

         if(strncasecmp(buf, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            state = SMTP_STATE_FINISHED;

            snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_221_GOODBYE, cfg.hostid);
            send(QC->sockfd, buf, strlen(buf), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

            unlink(sdata.ttmpfile);

            goto QUITTING;
         }

         // NOOP

         if(strncasecmp(buf, SMTP_CMD_NOOP, strlen(SMTP_CMD_NOOP)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            send(QC->sockfd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);

            continue;
         }

         // RSET

         if(strncasecmp(buf, SMTP_CMD_RESET, strlen(SMTP_CMD_RESET)) == 0){
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

            /* we must send a 250 Ok */

            send(QC->sockfd, SMTP_RESP_250_OK, strlen(SMTP_RESP_250_OK), 0);
            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_250_OK);

            /* remove old queue file, 2007.07.17, SJ */

            if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: removed", sdata.ttmpfile);
            unlink(sdata.ttmpfile);


            /* initialise data */

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

            for(i=0; i<MAX_RCPT_TO; i++)
               memset(sdata.rcptto[i], 0, MAXBUFSIZE);

            gettimeofday(&tv_start, &tz);

            make_rnd_string(&(sdata.ttmpfile[0]));
            unlink(sdata.ttmpfile);

            /* end of initialisation */


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
                     extract_email(sdata.rcptto[i], email);
                     snprintf(buf, MAXBUFSIZE-1, "421 writing queue file failed %s <%s>\r\n", sdata.ttmpfile, email);
                     send(QC->sockfd, buf, strlen(buf), 0);               
                  }
               #else
                  send(QC->sockfd, SMTP_RESP_421_ERR_WRITE_FAILED, strlen(SMTP_RESP_421_ERR_WRITE_FAILED), 0);
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

               ret = cl_scanfile(sdata.ttmpfile, &virname, NULL, engine, &limits, options);

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
               memset(whitelistbuf, 0, SMALLBUFSIZE);
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
                  result.spaminess = DEFAULT_SPAMICITY;

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


                  #ifdef HAVE_MYSQL
                     if(db_connection == 1){
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
                           if(is_sender_on_white_list(mysql, email, sdata.uid)){
                              syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata.ttmpfile, email);
                              snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on white list\r\n", cfg.clapf_header_field);
                              goto END_OF_SPAM_CHECK;
                           }
                           else {
                              if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: running Bayesian test", sdata.ttmpfile);
                              result = bayes_file(mysql, spamfile, sstate, sdata, cfg);
                           }
                           update_mysql_tokens(mysql, sstate.first, sdata.uid);

                           if(
                               (cfg.training_mode == T_TUM && ( (result.spaminess >= cfg.spam_overall_limit && result.spaminess < 0.99) || (result.spaminess < cfg.max_ham_spamicity && result.spaminess > 0.1) )) 
                               ||
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

                     }
                     else {
                        result.spaminess = DEFAULT_SPAMICITY;
                     }

                     gettimeofday(&tv_spam_stop, &tz);
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

                     #ifdef HAVE_MYSQL
                        insert_2_queue(mysql, sdata.ttmpfile, sdata.uid, cfg, is_spam);
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

                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s%s%s%s\r\n",
                           cfg.clapf_header_field, result.spaminess, cfg.clapf_header_field, sdata.ttmpfile, reason, trainbuf, whitelistbuf, cfg.clapf_spam_header_field);


                        log_ham_spam_per_email(sdata.ttmpfile, email, 1);
                     }
                     else {
                        snprintf(spaminessbuf, MAXBUFSIZE-1, "%s%.4f\r\n%s%s\r\n%s%s", cfg.clapf_header_field, result.spaminess, cfg.clapf_header_field, sdata.ttmpfile, trainbuf, whitelistbuf);

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
                  send(QC->sockfd, acceptbuf, strlen(acceptbuf), 0);

                  if(inj == ERR_DROP_SPAM) syslog(LOG_PRIORITY, "%s: dropped spam", sdata.ttmpfile);
                  else if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, acceptbuf);

            #ifdef HAVE_LMTP
               } /* for */
            #endif


               /* close database backend handler */

            #ifdef HAVE_ANTISPAM
               free_and_print_list(sstate.first, 0);
            #endif

            } /* PERIOD found */

         AFTER_PERIOD:

            memcpy(prevbuf, buf, n);
            prevlen = n;

            continue;

         } /* SMTP DATA */


      /* by default send 502 command not implemented message */

      send(QC->sockfd, SMTP_RESP_502_ERR, strlen(SMTP_RESP_502_ERR), 0);
      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, SMTP_RESP_502_ERR);

   } /* while */



   /*
    * TODO:
    *       establish a timeout mechanism, if l/smtp client is waiting too much
    *
    */

QUITTING:

#ifdef HAVE_MYSQL
   mysql_close(&mysql);
#endif

   close(QC->sockfd);

   decrement_thread_count();
   pthread_exit(0);

   return 0;
}


int main(int argc, char **argv){
   int i, fdmax, newfd, addrlen, yes=1, daemonise=0;
   struct sockaddr_in remote_addr;
   struct sockaddr_in local_addr;
   struct in_addr addr;
   struct timeval tv;
   qconn *QC;
   fd_set master, read_fds;
   FILE *f;


   while((i = getopt(argc, argv, "c:dVh")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'd' :
                    daemonise = 1;
                    break;

         case 'V' :
                    __fatal(PROGNAME " " VERSION);
                    break;

         case 'h' :
         default  : 
                    __fatal(CLAPFUSAGE);
                    break;

       }
   }


   /* initialize */


   (void) openlog(PROGNAME, LOG_PID, LOG_MAIL);

   signal(SIGINT, clapf_exit);
   signal(SIGQUIT, clapf_exit);
   signal(SIGKILL, clapf_exit);
   signal(SIGTERM, clapf_exit);
   signal(SIGHUP, reload_config);

#ifdef HAVE_LIBCLAMAV
   signal(SIGALRM, reload_clamav_db);
#endif

   reload_config();

   /* libclamav startup */

#ifdef HAVE_LIBCLAMAV
   reload_clamav_db();
#endif

   FD_ZERO(&master);
   FD_ZERO(&read_fds);

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   __num_threads=0;


   /* create a listener socket */

   if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      fatal(ERR_OPEN_SOCKET);

   local_addr.sin_family = AF_INET;
   local_addr.sin_port = htons(cfg.listen_port);
   inet_aton(cfg.listen_addr, &addr);
   local_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(local_addr.sin_zero), 8);

   if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      fatal(ERR_SET_SOCK_OPT);

   if(bind(listener, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1)
      fatal(ERR_BIND_TO_PORT);

   if(listen(listener, cfg.backlog) == -1)
      fatal(ERR_LISTEN);

   FD_SET(listener, &master);
   fdmax = listener;

   syslog(LOG_PRIORITY, "%s %s starting", PROGNAME, VERSION);

   /* write pid file */

   f = fopen(cfg.pidfile, "w");
   if(f){
      fprintf(f, "%d", getpid());
      fclose(f);
   }
   else syslog(LOG_PRIORITY, "cannot write pidfile: %s", cfg.pidfile);

   if(daemonise == 1) daemon(1, 0);


   /* main loop */
 
   for(;;) {
      read_fds = master;

      tv.tv_sec = 2;
      tv.tv_usec = 0;


      if(select(fdmax+1, &read_fds, NULL, NULL, &tv) > 0){

         for(i=0;i<=fdmax;i++) {
            if(FD_ISSET(i, &read_fds)){

               /* handle connections */

               if(i == listener){
                  addrlen = sizeof(remote_addr);

                  if((newfd = accept(listener, (struct sockaddr *)&remote_addr, (socklen_t *) &addrlen)) == -1){
                     fprintf(stderr, "daemon error accept\n");
                     continue;
                  }
                  fcntl(newfd, F_SETFL, O_RDWR);
                  setsockopt(newfd, SOL_SOCKET, TCP_NODELAY, &yes, sizeof(int));

                  if(__num_threads >= cfg.max_connections){
                     send(newfd, too_many_connections, strlen(too_many_connections), 0);
                     syslog(LOG_PRIORITY, "too many connections: %d/%d\n", __num_threads, cfg.max_connections);
                     goto DEFERRED;
                  }

                  QC = malloc(sizeof(qconn));
                  if(QC == NULL) goto DEFERRED;

                  increment_thread_count();
                  QC->sockfd = newfd;


                  if(pthread_create(&QC->thread, &attr, process_connection, (void *)QC)){
                     decrement_thread_count();

                  DEFERRED:
                     close(newfd);
                     continue;
                  }

               }
            }
         }
      }
   }



   return 0;
}
