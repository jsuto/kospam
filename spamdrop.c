/*
 * spamdrop.c, 2009.02.24, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#include <sysexits.h>
#include <clapf.h>
#include <locale.h>

extern char *optarg;
extern int optind;


#ifdef HAVE_MYSQL
   MYSQL_RES *res;
   MYSQL_ROW row;
#endif
#ifdef HAVE_SQLITE3
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;
#endif
#ifdef HAVE_MYDB
   #include "mydb.h"
#endif

/* open database connection */

int open_db(struct session_data *sdata, struct __config *cfg){
#ifdef HAVE_MYSQL
   mysql_init(&(sdata->mysql));
   mysql_options(&(sdata->mysql), MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg->mysql_connect_timeout);
   if(mysql_real_connect(&(sdata->mysql), cfg->mysqlhost, cfg->mysqluser, cfg->mysqlpwd, cfg->mysqldb, cfg->mysqlport, cfg->mysqlsocket, 0) == 0){
      syslog(LOG_PRIORITY, "%s: %s", sdata->ttmpfile, ERR_MYSQL_CONNECT);
      return 0;
   }
#endif

#ifdef HAVE_SQLITE3
   int rc;

   rc = sqlite3_open(cfg->sqlite3, &(sdata->db));
   if(rc){
      syslog(LOG_PRIORITY, "%s: %s: %s", sdata->ttmpfile, ERR_SQLITE3_OPEN, cfg->sqlite3);
      return 0;
   }
   else {
      rc = sqlite3_exec(sdata->db, cfg->sqlite3_pragma, 0, 0, NULL);
      if(rc != SQLITE_OK) syslog(LOG_PRIORITY, "%s: could not set pragma", sdata->ttmpfile);
   }
#endif

#ifdef HAVE_MYDB
   int rc;

   rc = init_mydb(cfg->mydbfile, sdata);
   if(rc != 1)
      return 0;
#endif

   return 1;
}


int main(int argc, char **argv, char **envp){
   FILE *f, *ofile=stdout;
   int i, n, fd, tot_len=0, rc=0, is_header=1, rounds=1, deliver_message=0;
   int print_message=1, print_summary_only=0, put_subject_spam_prefix=0, sent_subject_spam_prefix=0;
   int is_spam=0, train_as_ham=0, train_as_spam=0, blackhole_request=0, training_request=0;
   int train_mode=T_TOE;
   uid_t u=-1;
   char buf[MAXBUFSIZE], qpath[SMALLBUFSIZE], trainbuf[SMALLBUFSIZE], ID[RND_STR_LEN+1], whitelistbuf[SMALLBUFSIZE], clapf_info[MAXBUFSIZE];
   char *configfile=CONFIG_FILE, *username=NULL, *from=NULL, *recipient=NULL;
   struct timezone tz;
   struct timeval tv_start, tv_stop;
   struct passwd *pwd;
   struct session_data sdata;
   struct _state state;
   struct __config cfg;
   float spaminess=DEFAULT_SPAMICITY;
#ifdef MY_TEST
   char rblbuf[SMALLBUFSIZE];
#endif
#ifdef HAVE_LANG_DETECT
   char *lang="unknown";
#endif
#ifdef HAVE_SPAMDROP_HELPER
   char envvar[SMALLBUFSIZE];
   char *eeenv[] = { NULL, (char *) 0 };
#endif
#ifdef HAVE_SPAMSUM
   char *sum, spamsum_buf[SMALLBUFSIZE];
   unsigned int flags=0, spamsum_score=0;
#endif


   while((i = getopt(argc, argv, "c:u:U:f:r:SHsdh?")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'u' :
                    username = optarg;
                    break;

         case 'U' :
                    u = atoi(optarg);
                    break;

         case 'f' :
                    from = optarg;
                    break;

         case 'r' :
                    recipient = optarg;
                    break;

         case 'd' :
                    deliver_message = 1;
                    break;

         case 's' :
                    print_summary_only = 1;
                    break;

         case 'S' :
                    train_as_spam = 1;
                    break;

         case 'H' :
                    train_as_ham = 1;
                    break;

         case 'h' :
         case '?' :
                    printf("%s", SPAMDROPUSAGE);
                    break;


         default  : 
                    break;
       }
   }


   (void) openlog("spamdrop", LOG_PID, LOG_MAIL);

   if(strstr(argv[0], "blackhole")) blackhole_request = 1;

   if(train_as_spam == 1 && train_as_ham == 1){
      fprintf(stderr, "%s\n", ERR_TRAIN_AS_HAMSPAM);
      return 0;
   }

   /* read config file */

   cfg = read_config(configfile);

   setlocale(LC_ALL, cfg.locale);

   if(u < 0) u = getuid();

   /* do not query the username if we got it from the command line, 2008.03.10, SJ */

   if(username == NULL){

      /* maildrop exports the LOGNAME environment variable */

      username = getenv("LOGNAME");
      if(!username){
         pwd = getpwuid(getuid());
         username = pwd->pw_name;
      }
   }

#ifdef HAVE_SQLITE3
   if(strlen(cfg.sqlite3) < 4)
      snprintf(cfg.sqlite3, MAXVAL-1, "%s/%s/%c/%s/%s", cfg.chrootdir, USER_DATA_DIR, username[0], username, PER_USER_SQLITE3_DB_FILE);
#endif
#ifdef HAVE_MYDB
   if(strlen(cfg.mydbfile) < 4)
      snprintf(cfg.mydbfile, MAXVAL-1, "%s/%s/%c/%s/%s", cfg.chrootdir, USER_DATA_DIR, username[0], username, MYDB_FILE);
#endif


   /* check for the queue directory, and run the helper script, if we have to */

#ifdef HAVE_SPAMDROP_HELPER
   snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username);

   if(stat(buf, &st) != 0){
      syslog(LOG_PRIORITY, "running spamdrop helper script: %s, for user: %s", SPAMDROP_HELPER_PROGRAM, username);

      snprintf(envvar, SMALLBUFSIZE-1, "YOURUSERNAME=%s", username);
      putenv(envvar);
      eeenv[0] = &envvar[0];

      execl(SPAMDROP_HELPER_PROGRAM, envvar, (char*)0);

      if(stat(buf, &st) != 0){
         syslog(LOG_PRIORITY, "missing user directory: %s", buf);
         return EX_TEMPFAIL;
      }
   }
#endif

   /* do not go to the workdir if this is a cmdline training request */

   if(train_as_ham == 0 && train_as_spam == 0)
      chdir(cfg.workdir);


   if(!from) from = getenv("FROM");

   sdata.num_of_rcpt_to = 1;
   sdata.uid = u;
   sdata.Nham = sdata.Nspam = 0;
   if(from) snprintf(sdata.mailfrom, SMALLBUFSIZE-1, "%s", from);
   memset(sdata.rcptto[0], 0, SMALLBUFSIZE);
   memset(whitelistbuf, 0, SMALLBUFSIZE);
   memset(sdata.ttmpfile, 0, SMALLBUFSIZE);
   make_rnd_string(&(sdata.ttmpfile[0]));

   memset(qpath, 0, SMALLBUFSIZE);

   if(from && (strcasecmp(from, "MAILER-DAEMON") == 0 || strcmp(from, "<>") == 0) && strlen(cfg.our_signo) > 3){
      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: from: %s, we should really see our signo", sdata.ttmpfile, from);
      sdata.need_signo_check = 1;
   }

#ifdef HAVE_SQLITE3
   sdata.uid = 0;
#endif

   memset(trainbuf, 0, SMALLBUFSIZE);


   fd = open(sdata.ttmpfile, O_CREAT|O_EXCL|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
   if(fd == -1){
      close(fd);
      syslog(LOG_PRIORITY, "cannot open: %s", sdata.ttmpfile);
      return EX_TEMPFAIL;
   }

   /* read message from standard input */

   while((n = read(0, buf, MAXBUFSIZE)) > 0){
      tot_len += n;
      write(fd, buf, n);
   }


   /* make sure we had a successful read */

   if(fsync(fd)){
      syslog(LOG_PRIORITY, "failed writing data: %s", sdata.ttmpfile);
      return EX_TEMPFAIL;
   }

   close(fd);

   gettimeofday(&tv_start, &tz);


#ifdef HAVE_ANTIVIRUS
   if(do_av_check(&sdata, recipient, from, &cfg) == AVIR_VIRUS){
      syslog(LOG_PRIORITY, "%s: dropping infected message", sdata.ttmpfile);
      goto ENDE;
   }
#endif


   /* skip spamicity check if message is too long */
   if( (print_message == 1 || print_summary_only == 1) && tot_len > cfg.max_message_size_to_filter){
      gettimeofday(&tv_stop, &tz);
      goto ENDE_SPAMDROP;
   }

   /*******************************************************************************/
   /* check whether this is a training request with user+spam@... or user+ham@... */
   /***************************************************************************** */

   f = fopen(sdata.ttmpfile, "r");
   if(f){
      while(fgets(buf, MAXBUFSIZE-1, f)){
         if(strncmp(buf, "To:", 3) == 0 && (strcasestr(buf, "+ham@") || strcasestr(buf, "+spam@")) ){
            trim(buf);
            syslog(LOG_PRIORITY, "training request: %s", buf);
            training_request = 1;
            break;
         }

         if(buf[0] == '\r' || buf[0] == '\n') break;
      }
      fclose(f);
   }


   /* open database connection */
   if(open_db(&sdata, &cfg) == 0)
      goto ENDE;


   /****************************/
   /* handle training requests */
   /****************************/


   if(training_request == 1){
      rounds = MAX_ITERATIVE_TRAIN_LOOPS;

      if(!from) goto CLOSE_DB;

      is_spam = 0;
      if(strcasestr(buf, "+spam@")) is_spam = 1;

      /* determine the queue file from the message */
      train_mode = extract_id_from_message(sdata.ttmpfile, cfg.clapf_header_field, ID);

      /* determine the path of the original file */

      if(is_spam == 1)
         snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s/h.%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username, ID);
      else
         snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s/s.%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username, ID);


      state = parse_message(buf, &sdata, &cfg);

      /* is it a TUM trained message? */

      if(state.train_mode == T_TUM)
         train_mode=T_TUM;

      /* ... then train with the message */

      train_message(&sdata, &state, rounds, is_spam, train_mode, &cfg);

      goto CLOSE_DB;
   }


   /* parse message */
   state = parse_message(sdata.ttmpfile, &sdata, &cfg);


   /*******************************************************/
   /* if this is a training request from the command line */
   /***************************************************** */

   if(train_as_ham == 1 || train_as_spam == 1){
      if(train_as_spam == 1) is_spam = 1;
      else is_spam = 0;

      /* is it a TUM trained message? */

      if(state.train_mode == T_TUM)
         train_mode=T_TUM;

   #ifdef HAVE_USERS
      if(from) get_user_from_email(&sdata, from, &cfg);
   #else
      sdata.uid = 0;
   #endif

      if(cfg.group_type == GROUP_SHARED) sdata.uid = 0;

      train_message(&sdata, &state, rounds, is_spam, train_mode, &cfg);
   }


   /*
    * or just calculate spamicity
    */

   else {

      /* whitelist check first */

   #ifdef HAVE_WHITELIST
      if(is_sender_on_black_or_white_list(&sdata, from, SQL_WHITE_LIST, &cfg)){
         syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata.ttmpfile, from);
         snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on whitelist\r\n", cfg.clapf_header_field);
         goto ENDE_SPAMDROP;
      }
   #endif

      /* then give blacklist a try */

   #ifdef HAVE_BLACKLIST
      if(is_sender_on_black_or_white_list(&sdata, from, SQL_BLACK_LIST, &cfg) == 1){
         syslog(LOG_PRIORITY, "%s: sender (%s) found on blacklist", sdata.ttmpfile, from);
         snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on blacklist\r\n", cfg.clapf_header_field);
         goto ENDE;
      }
   #endif


      /* query spaminess */

      spaminess = bayes_file(&sdata, &state, &cfg);

      /* update tokens */

      if(cfg.update_tokens == 1){
      #ifdef HAVE_MYSQL
         update_mysql_tokens(sdata.mysql, state.token_hash, sdata.uid);
      #endif
      #ifdef HAVE_SQLITE3
         update_sqlite3_tokens(sdata.db, state.token_hash);
      #endif
      #ifdef HAVE_MYDB
         update_tokens(cfg.mydbfile, sdata.mhash, state.token_hash);
      #endif
      }

   #ifdef HAVE_LANG_DETECT
      lang = check_lang(state.token_hash);
   #endif

   #ifdef HAVE_SPAMSUM
      /* if we are uncertain, try the spamsum module, 2008.04.28, SJ */

      if(spaminess > cfg.max_ham_spamicity && spaminess < cfg.spam_overall_limit){
         flags |= FLAG_IGNORE_HEADERS;
         sum = spamsum_file(sdata.ttmpfile, flags, 0);
         if(sum){
            spamsum_score = spamsum_match_db(cfg.sig_db, sum, 55);
            if(spamsum_score >= 50) spaminess = 0.9988;
            snprintf(spamsum_buf, SMALLBUFSIZE-1, "%sspamsum=%d\r\n", cfg.clapf_header_field, spamsum_score);
            free(sum);
         }
      }
   #endif

      if(sdata.need_signo_check == 1){
         if(!state.found_our_signo){
            syslog(LOG_PRIORITY, "%s: looks like a bounce, but our signo is missing", sdata.ttmpfile);
         }
         else {
            syslog(LOG_PRIORITY, "found our signo, this should be a real bounce message");
            spaminess = DEFAULT_SPAMICITY;
         }
      }


      if(spaminess >= cfg.spam_overall_limit)
         is_spam = 1;
      else
         is_spam = 0;


      /* don't TUM train if this is a blackhole message */

      if(
         (blackhole_request == 0 && cfg.training_mode == T_TUM && ( (spaminess >= cfg.spam_overall_limit && spaminess < 0.99) || (spaminess < cfg.max_ham_spamicity && spaminess > 0.1) )) ||
         (cfg.initial_1000_learning == 1 && (sdata.Nham < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED || sdata.Nspam < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED))
        )
      {

         if(is_spam == 1)
            syslog(LOG_PRIORITY, "%s: TUM training a spam", sdata.ttmpfile);
         else
            syslog(LOG_PRIORITY, "%s: TUM training a ham", sdata.ttmpfile);

         snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg.clapf_header_field);

         train_message(&sdata, &state, 1, is_spam, train_mode, &cfg);
      }

   }

   /****************************************************************************************************/
   /* if this is a blackhole request and spaminess < 0.99, then learn the message in an iterative loop */
   /****************************************************************************************************/

   if(blackhole_request == 1){
      if(spaminess < 0.99){
         rounds = MAX_ITERATIVE_TRAIN_LOOPS;

         syslog(LOG_PRIORITY, "%s: training on a blackhole message", sdata.ttmpfile);
         snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM on blackhole\r\n", cfg.clapf_header_field);

         train_message(&sdata, &state, rounds, 1, T_TOE, &cfg);
      }

   #ifdef HAVE_BLACKHOLE
      state.ip[strlen(state.ip)-1] = '\0';
      put_ip_to_dir(cfg.blackhole_path, state.ip);
   #endif
   }


   /* close db handles */

CLOSE_DB:

#ifdef HAVE_MYSQL
   mysql_close(&(sdata.mysql));
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(sdata.db);
#endif
#ifdef HAVE_MYDB
   close_mydb(sdata.mhash);
#endif

   /* free structures */
   free_list(state.urls);
   clearhash(state.token_hash, 0);

   gettimeofday(&tv_stop, &tz);


   /* save email for later retraining and/or spam quarantine */

#ifdef HAVE_STORE
   if(tot_len <= cfg.max_message_size_to_filter && blackhole_request == 0)
      save_email_to_queue(&sdata, spaminess, &cfg);
#endif


ENDE_SPAMDROP:

   syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, spaminess, tot_len, tvdiff(tv_stop, tv_start)/1000);


   /********************************/
   /* print message to stdout/pipe */
   /********************************/

   if(print_message == 1){
      f = fopen(sdata.ttmpfile, "r");
      if(!f){
         syslog(LOG_PRIORITY, "cannot read: %s", sdata.ttmpfile);
         return EX_TEMPFAIL;
      }

      if(spaminess >= cfg.spam_overall_limit && spaminess < 1.01 && strlen(cfg.spam_subject_prefix) > 1) put_subject_spam_prefix = 1;

   #ifdef MY_TEST
      memset(rblbuf, 0, SMALLBUFSIZE);
      reverse_ipv4_addr(state.ip);
      if(rbl_list_check("zen.spamhaus.org", state.ip, cfg.verbosity) == 1)
         snprintf(rblbuf, SMALLBUFSIZE-1, "%sZEN=1\r\n", cfg.clapf_header_field);
      else
         snprintf(rblbuf, SMALLBUFSIZE-1, "%sZEN=0\r\n", cfg.clapf_header_field);

   #endif

      /* assemble clapf header info */

      memset(clapf_info, 0, MAXBUFSIZE);

      snprintf(clapf_info, MAXBUFSIZE-1, "%s%s\r\n%s%s%.4f\r\n%s%ld ms\r\n",
              cfg.clapf_header_field, sdata.ttmpfile, trainbuf, cfg.clapf_header_field, spaminess, cfg.clapf_header_field, tvdiff(tv_stop, tv_start)/1000);

      if(spaminess > 0.9999){
         strncat(clapf_info, cfg.clapf_header_field, MAXBUFSIZE-1);
         strncat(clapf_info, MSG_ABSOLUTELY_SPAM, MAXBUFSIZE-1);
         strncat(clapf_info, "\r\n", MAXBUFSIZE-1);
      }

      if(spaminess >= cfg.spam_overall_limit && spaminess < 1.01){
         strncat(clapf_info, cfg.clapf_header_field, MAXBUFSIZE-1);
         strncat(clapf_info, "Yes\r\n", MAXBUFSIZE-1);
      }


   #ifdef MY_TEST
      strncat(clapf_info, rblbuf, MAXBUFSIZE-1);
   #endif
   #ifdef HAVE_SPAMSUM
      strncat(clapf_info, spamsum_buf, MAXBUFSIZE-1);
   #endif
   #ifdef HAVE_LANG_DETECT
      strncat(clapf_info, cfg.clapf_header_field, MAXBUFSIZE-1);
      strncat(clapf_info, lang, MAXBUFSIZE-1);
      strncat(clapf_info, "\r\n", MAXBUFSIZE-1);
   #endif
   #ifdef HAVE_WHITELIST
      strncat(clapf_info, whitelistbuf, MAXBUFSIZE-1);
   #endif


      if(deliver_message == 1){
         snprintf(buf, MAXBUFSIZE-1, "%s %s %s", cfg.delivery_agent, from, recipient);
         ofile = popen(buf, "w");
      }

      fprintf(ofile, "%s", clapf_info);

      while(fgets(buf, MAXBUFSIZE-1, f)){

         /* tag the Subject line if we have to, 2007.08.21, SJ */

         if(is_header == 1 && put_subject_spam_prefix == 1 && strncmp(buf, "Subject:", 8) == 0 && !strstr(buf, cfg.spam_subject_prefix)){
            fprintf(ofile, "Subject: ");
            fprintf(ofile, "%s", cfg.spam_subject_prefix);
            fprintf(ofile, "%s", &buf[9]);
            sent_subject_spam_prefix = 1;

            continue;
         }

         if(is_header == 1 && (buf[0] == '\n' || buf[0] == '\r')){
            is_header = 0;

            /* if we did not find a Subject line, add one - if we have to */

            if(sent_subject_spam_prefix == 0 && put_subject_spam_prefix == 1 && spaminess >= cfg.spam_overall_limit && spaminess < 1.01)
               fprintf(ofile, "Subject: %s\r\n", cfg.spam_subject_prefix);

         }

         if(strncmp(buf, cfg.clapf_header_field, strlen(cfg.clapf_header_field)))
            fprintf(ofile, "%s", buf);
      }

      fclose(stdout);

   }


   snprintf(buf, MAXBUFSIZE-1, "%ld", sdata.uid);

   /*******************************/
   /* print summary if we have to */
   /*******************************/

   if(spaminess >= cfg.spam_overall_limit){
      rc = 1;
      if(print_summary_only == 1)
         printf("S %.4f\n", spaminess);

      log_ham_spam_per_email(sdata.ttmpfile, buf, 1);
   }
   else {
      rc = 0;
      if(print_summary_only == 1){
         if(spaminess <= cfg.max_ham_spamicity)
            printf("H %.4f\n", spaminess);
         else
            printf("U %.4f\n", spaminess);
      }

      log_ham_spam_per_email(sdata.ttmpfile, buf, 0);
   }


ENDE:

   /* unlink temp file */
   unlink(sdata.ttmpfile);


   /* add trailing dot to the file, 2008.09.08, SJ */

   if(strlen(qpath) > 3){
      fd = open(qpath, O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
      if(fd != -1){
         lseek(fd, 0, SEEK_END);
         write(fd, SMTP_CMD_PERIOD, strlen(SMTP_CMD_PERIOD));
         close(fd);
      }
   }

   if(print_message == 0 && spaminess >= cfg.spam_overall_limit && spaminess < 1.01)
      return 1;

   /* maildrop requires us to exit with 0 */
   return 0;
}
