/*
 * spamdrop.c, 2008.05.09, SJ
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

extern char *optarg;
extern int optind;


struct __config cfg;
struct session_data sdata;
struct c_res result;


#ifdef HAVE_MYSQL
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;
   struct ue UE;
#endif
#ifdef HAVE_SQLITE3
   sqlite3 *db;
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;
#endif
#ifdef HAVE_MYDB
   struct mydb_node *mhash[MAX_MYDB_HASH];
#endif


/* open database connection */

int open_db(char *messagefile){
#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0) == 0){
      syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_MYSQL_CONNECT);
      return 0;
   }
#endif

#ifdef HAVE_SQLITE3
   int rc;

   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc){
      syslog(LOG_PRIORITY, "%s: %s: %s", messagefile, ERR_SQLITE3_OPEN, cfg.sqlite3);
      return 0;
   }
   else {
      rc = sqlite3_exec(db, cfg.sqlite3_pragma, 0, 0, NULL);
      if(rc != SQLITE_OK) syslog(LOG_PRIORITY, "%s: could not set pragma", sdata.ttmpfile);
   }
#endif

#ifdef HAVE_MYDB
   int rc;

   rc = init_mydb(cfg.mydbfile, mhash);
   if(rc != 1)
      return 0;
#endif

   return 1;
}


int main(int argc, char **argv, char **envp){
   FILE *f;
   int i, n, fd, fd2, tot_len=0, rc=0, is_header=1, rounds=1;
   int print_message=0, print_summary_only=0, put_subject_spam_prefix=0, sent_subject_spam_prefix=0;
   int is_spam=0, train_as_ham=0, train_as_spam=0, blackhole_request=0, training_request=0;
   int train_mode=T_TOE;
   uid_t u;
   char buf[MAXBUFSIZE], qpath[SMALLBUFSIZE], trainbuf[SMALLBUFSIZE], ID[RND_STR_LEN+1], whitelistbuf[SMALLBUFSIZE];
   char *configfile=CONFIG_FILE, *username=NULL, *from=NULL;
   struct timezone tz;
   struct timeval tv_start, tv_stop;
   struct stat st;
   struct passwd *pwd;
   struct _state state;

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


   while((i = getopt(argc, argv, "c:u:SHps")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'u' :
                    username = optarg;
                    break;

         case 'p' :
                    print_message = 1;
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

         default  : 
                    break;
       }
   }


   (void) openlog("spamdrop", LOG_PID, LOG_MAIL);

#ifdef HAVE_BLACKHOLE
   if(strstr(argv[0], "blackhole")) blackhole_request = 1;
#endif

   if(train_as_spam == 1 && train_as_ham == 1){
      fprintf(stderr, "%s\n", ERR_TRAIN_AS_HAMSPAM);
      return 0;
   }

   /* read config file */

   cfg = read_config(configfile);

   /* do not query the username if we got it from the command line, 2008.03.10, SJ */

   if(username == NULL){

      /* maildrop exports the LOGNAME environment variable */

      username = getenv("LOGNAME");
      if(!username){
         u = getuid();
         pwd = getpwuid(u);
         username = pwd->pw_name;
      }
   }

   snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username);

#ifdef HAVE_SQLITE3
   if(strlen(cfg.sqlite3) < 4)
      snprintf(cfg.sqlite3, MAXVAL-1, "%s/%s/%c/%s/%s", cfg.chrootdir, USER_DATA_DIR, username[0], username, PER_USER_SQLITE3_DB_FILE);
#endif
#ifdef HAVE_MYDB
   if(strlen(cfg.mydbfile) < 4)
      snprintf(cfg.mydbfile, MAXVAL-1, "%s/%s/%c/%s/%s", cfg.chrootdir, USER_DATA_DIR, username[0], username, MYDB_FILE);
#endif


   /* check for the queue directory, and run the helper script, if we have to, 2008.04.12, SJ */

#ifdef HAVE_SPAMDROP_HELPER
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


   if(cfg.store_metadata == 1){
      if(chdir(buf)){
         syslog(LOG_PRIORITY, "cannot chdir to %s", buf);
         return EX_TEMPFAIL;
      }
   }

   from = getenv("FROM");

   sdata.num_of_rcpt_to = 1;
   sdata.uid = getuid();
   if(from) snprintf(sdata.mailfrom, MAXBUFSIZE-1, "%s", from);
   memset(sdata.rcptto[0], 0, MAXBUFSIZE);
   memset(whitelistbuf, 0, SMALLBUFSIZE);
   make_rnd_string(&(sdata.ttmpfile[0]));

   result.spaminess = DEFAULT_SPAMICITY;
   result.ham_msg = result.spam_msg = 0;

   if(from && (strcasecmp(from, "MAILER-DAEMON") == 0 || strcmp(from, "<>") == 0) && strlen(cfg.our_signo) > 3){
      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "from: %s, we should really see our signo", from);
      sdata.need_signo_check = 1;
   }

#ifdef HAVE_SQLITE3
   sdata.uid = 0;
#endif

   memset(trainbuf, 0, SMALLBUFSIZE);


   /* read message from standard input */

   fd = open("/dev/stdin", O_RDONLY);
   if(fd == -1){
      syslog(LOG_PRIORITY, "cannot read stdin");
      return EX_TEMPFAIL;
   }

   fd2 = open(sdata.ttmpfile, O_CREAT|O_EXCL|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
   if(fd2 == -1){
      close(fd);
      syslog(LOG_PRIORITY, "cannot open: %s", sdata.ttmpfile);
      return EX_TEMPFAIL;
   }

   while((n = read(fd, buf, MAXBUFSIZE)) > 0){
      tot_len += n;
      write(fd2, buf, n);
   }

   close(fd);

   /* make sure we had a successful read */

   if(fsync(fd2)){
      syslog(LOG_PRIORITY, "failed writing data: %s", sdata.ttmpfile);
      return EX_TEMPFAIL;
   }

   close(fd2);

   gettimeofday(&tv_start, &tz);


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
         if(strncmp(buf, "To:", 3) == 0 && (str_case_str(buf, "+ham@") || str_case_str(buf, "+spam@")) ){
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
   if(open_db(sdata.ttmpfile) == 0)
      goto ENDE;


   /****************************/
   /* handle training requests */
   /****************************/


   if(training_request == 1){
      rounds = MAX_ITERATIVE_TRAIN_LOOPS;

      if(!from) goto CLOSE_DB;

      is_spam = 0;
      if(str_case_str(buf, "+spam@")) is_spam = 1;

      /* determine the queue file from the message */
      train_mode = extract_id_from_message(sdata.ttmpfile, cfg.clapf_header_field, ID);

      /* determine the path of the original file */

      if(is_spam == 1)
         snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s/h.%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username, ID);
      else
         snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s/s.%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username, ID);


      state = parse_message(buf, sdata, cfg);

      /* is it a TUM trained message? */

      if(state.train_mode == T_TUM)
         train_mode=T_TUM;

      /* ... then train with the message */

   #ifdef HAVE_MYSQL
      train_message(mysql, sdata, state, rounds, is_spam, train_mode, cfg);
   #endif
   #ifdef HAVE_SQLITE3
      train_message(db, sdata, state, rounds, is_spam, train_mode, cfg);
   #endif
   #ifdef HAVE_MYDB
      train_message(cfg.mydbfile, mhash, sdata, state, rounds, is_spam, train_mode, cfg);
   #endif

      goto CLOSE_DB;
   }


   /* parse message */
   state = parse_message(sdata.ttmpfile, sdata, cfg);


   /*******************************************************/
   /* if this is a training request from the command line */
   /***************************************************** */

   if(train_as_ham == 1 || train_as_spam == 1){
      if(train_as_spam == 1) is_spam = 1;
      else is_spam = 0;

      /* is it a TUM trained message? */

      if(state.train_mode == T_TUM)
         train_mode=T_TUM;


   #ifdef HAVE_MYSQL
      train_message(mysql, sdata, state, rounds, is_spam, train_mode, cfg);
   #endif
   #ifdef HAVE_SQLITE3
      train_message(db, sdata, state, rounds, is_spam, train_mode, cfg);
   #endif
   #ifdef HAVE_MYDB
      train_message(cfg.mydbfile, mhash, sdata, state, rounds, is_spam, train_mode, cfg);
   #endif
   }


   /*
    * or just calculate spamicity
    */

   else {
   #ifdef HAVE_MYSQL
      if(is_sender_on_white_list(mysql, from, sdata.uid)){
         syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata.ttmpfile, from);
         snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on white list\r\n", cfg.clapf_header_field);
      } else
         result = bayes_file(mysql, sdata.ttmpfile, state, sdata, cfg);

      update_mysql_tokens(mysql, state.first, sdata.uid);
   #endif
   #ifdef HAVE_SQLITE3
      if(is_sender_on_white_list(db, from, sdata.uid)){
         syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata.ttmpfile, from);
         snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on white list\r\n", cfg.clapf_header_field);
      } else
         result = bayes_file(db, sdata.ttmpfile, state, sdata, cfg);

      update_sqlite3_tokens(db, state.first);
   #endif
   #ifdef HAVE_MYDB
      result = bayes_file(mhash, sdata.ttmpfile, state, sdata, cfg);
      update_tokens(cfg.mydbfile, mhash, state.first);
   #endif

   #ifdef HAVE_LANG_DETECT
      lang = check_lang(state.first);
   #endif

   #ifdef HAVE_SPAMSUM
      /* if we are uncertain, try the spamsum module, 2008.04.28, SJ */

      if(result.spaminess > cfg.max_ham_spamicity && result.spaminess < cfg.spam_overall_limit){
         flags |= FLAG_IGNORE_HEADERS;
         sum = spamsum_file(sdata.ttmpfile, flags, 0);
         if(sum){
            spamsum_score = spamsum_match_db(cfg.sig_db, sum, 55);
            if(spamsum_score >= 50) result.spaminess = 0.9988;
            snprintf(spamsum_buf, SMALLBUFSIZE-1, "%sspamsum=%d\r\n", cfg.clapf_header_field, spamsum_score);
            free(sum);
         }
      }
   #endif

      if(sdata.need_signo_check == 1){
         if(state.found_our_signo == 1)
            syslog(LOG_PRIORITY, "found our signo, this should be a real bounce message");
         else
            syslog(LOG_PRIORITY, "looks like a bounce, but our signo is missing");
      }

      if(result.spaminess >= cfg.spam_overall_limit)
         is_spam = 1;
      else
         is_spam = 0;
      

      if(
         (cfg.training_mode == T_TUM && ( (result.spaminess >= cfg.spam_overall_limit && result.spaminess < 0.99) || (result.spaminess < cfg.max_ham_spamicity && result.spaminess > 0.1) )) ||
         (cfg.initial_1000_learning == 1 && (result.ham_msg < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED || result.spam_msg < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED))
        )
      {

         if(is_spam == 1)
            syslog(LOG_PRIORITY, "%s: TUM training a spam", sdata.ttmpfile);
         else
            syslog(LOG_PRIORITY, "%s: TUM training a ham", sdata.ttmpfile);

         snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM\r\n", cfg.clapf_header_field);

         #ifdef HAVE_MYSQL
            train_message(mysql, sdata, state, 1, is_spam, train_mode, cfg);
         #endif
         #ifdef HAVE_SQLITE3
            train_message(db, sdata, state, 1, is_spam, train_mode, cfg);
         #endif
         #ifdef HAVE_MYDB
            train_message(cfg.mydbfile, mhash, sdata, state, 1, is_spam, train_mode, cfg);
         #endif
      }

   }

   /****************************************************************************************************/
   /* if this is a blackhole request and spaminess < 0.99, then learn the message in an iterative loop */
   /****************************************************************************************************/

   if(blackhole_request == 1 && result.spaminess < 0.99){
      rounds = MAX_ITERATIVE_TRAIN_LOOPS;

   #ifdef HAVE_MYSQL
      train_message(mysql, sdata, state, rounds, 1, T_TOE, cfg);
   #endif
   #ifdef HAVE_SQLITE3
      train_message(db, sdata, state, rounds, 1, T_TOE, cfg);
   #endif
   #ifdef HAVE_MYDB
      train_message(cfg.mydbfile, mhash, sdata, state, rounds, 1, T_TOE, cfg);
   #endif

   #ifdef HAVE_BLACKHOLE
      put_ip_to_dir(cfg.blackhole_path, state.ip);
      syslog(LOG_PRIORITY, "%s: training on a blackhole message", sdata.ttmpfile);
   #endif
   }


   /* close db handles */

CLOSE_DB:

#ifdef HAVE_SQLITE3
   sqlite3_close(db);
#endif
#ifdef HAVE_MYDB
   close_mydb(mhash);
#endif

   /* free structures */
   free_and_print_list(state.first, 0);

   gettimeofday(&tv_stop, &tz);


   /* rename file name according to its spamicity status, unless its a blackhole request, 2007.12.22, SJ */

   if(cfg.store_metadata == 1 && tot_len <= cfg.max_message_size_to_filter && blackhole_request == 0){
      if(result.spaminess >= cfg.spam_overall_limit)
         snprintf(qpath, SMALLBUFSIZE-1, "s.%s", sdata.ttmpfile);
      else
         snprintf(qpath, SMALLBUFSIZE-1, "h.%s", sdata.ttmpfile);

      link(sdata.ttmpfile, qpath);
      if(stat(qpath, &st) == 0){
         if(S_ISREG(st.st_mode) == 1)
            chmod(qpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
      }

   #ifdef HAVE_MYSQL
      insert_2_queue(mysql, sdata.ttmpfile, sdata.uid, cfg, is_spam);
   #endif
   #ifdef HAVE_SQLITE3
      insert_2_queue(db, sdata.ttmpfile, sdata.uid, cfg, is_spam);
   #endif

   }

ENDE_SPAMDROP:

   syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, result.spaminess, tot_len, tvdiff(tv_stop, tv_start)/1000);


   /***************************/
   /* print message to stdout */
   /***************************/

   if(print_message == 1){
      f = fopen(sdata.ttmpfile, "r");
      if(!f){
         syslog(LOG_PRIORITY, "cannot read: %s", sdata.ttmpfile);
         return EX_TEMPFAIL;
      }

      if(result.spaminess >= cfg.spam_overall_limit && result.spaminess < 1.01 && strlen(cfg.spam_subject_prefix) > 1) put_subject_spam_prefix = 1;

   #ifdef MY_TEST
      memset(rblbuf, 0, SMALLBUFSIZE);
      reverse_ipv4_addr(state.ip);
      if(rbl_list_check("zen.spamhaus.org", state.ip) == 1)
         snprintf(rblbuf, SMALLBUFSIZE-1, "%sZEN=1\r\n", cfg.clapf_header_field);
      else
         snprintf(rblbuf, SMALLBUFSIZE-1, "%sZEN=0\r\n", cfg.clapf_header_field);

   #endif

      while(fgets(buf, MAXBUFSIZE-1, f)){

         /* tag the Subject line if we have to, 2007.08.21, SJ */

         if(is_header == 1 && put_subject_spam_prefix == 1 && strncmp(buf, "Subject:", 8) == 0 && !strstr(buf, cfg.spam_subject_prefix)){ 
            printf("Subject: ");
            printf("%s", cfg.spam_subject_prefix);
            printf("%s", &buf[9]);
            sent_subject_spam_prefix = 1;
            continue;
         }

         if(is_header == 1 && (buf[0] == '\n' || buf[0] == '\r')){
            is_header = 0;

         #ifdef MY_TEST
            printf("%s", rblbuf);
         #endif
         #ifdef HAVE_SPAMSUM
            printf("%s", spamsum_buf);
         #endif

            printf("%s%s\r\n", cfg.clapf_header_field, sdata.ttmpfile);
            printf("%s%s%.4f\r\n", trainbuf, cfg.clapf_header_field, result.spaminess);
         #ifdef HAVE_LANG_DETECT
            printf("%s%s\r\n", cfg.clapf_header_field, lang);
         #endif
            printf("%s%ld ms\r\n", cfg.clapf_header_field, tvdiff(tv_stop, tv_start)/1000);
         #ifdef HAVE_WHITELIST
            printf("%s", whitelistbuf);
         #endif
            if(result.spaminess > 0.9999) printf("%s%s\r\n", cfg.clapf_header_field, MSG_ABSOLUTELY_SPAM);
            if(result.spaminess >= cfg.spam_overall_limit && result.spaminess < 1.01){

               /* if we did not find a Subject line */

               if(sent_subject_spam_prefix == 0 && put_subject_spam_prefix == 1)
                  printf("Subject: %s\r\n", cfg.spam_subject_prefix);

               printf("%sYes\r\n", cfg.clapf_header_field);
            }
         }

         if(strncmp(buf, cfg.clapf_header_field, strlen(cfg.clapf_header_field)))
            printf("%s", buf);
      }

   }


   snprintf(buf, MAXBUFSIZE-1, "%ld", sdata.uid);

   /*******************************/
   /* print summary if we have to */
   /*******************************/

   if(result.spaminess >= cfg.spam_overall_limit){
      rc = 1;
      if(print_summary_only == 1)
         printf("S %.4f\n", result.spaminess);

      log_ham_spam_per_email(sdata.ttmpfile, buf, 1);
   }
   else {
      rc = 0;
      if(print_summary_only == 1){
         if(result.spaminess <= cfg.max_ham_spamicity)
            printf("H %.4f\n", result.spaminess);
         else
            printf("U %.4f\n", result.spaminess);
      }

      log_ham_spam_per_email(sdata.ttmpfile, buf, 0);
   }


ENDE:

   /* unlink temp file */
   unlink(sdata.ttmpfile);

   /* add trailing dot to the file, 2008.01.27, SJ */

   if(cfg.store_metadata == 1 && tot_len <= cfg.max_message_size_to_filter && blackhole_request == 0){
      fd2 = open(qpath, O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
      if(fd2 != -1){
         lseek(fd2, 0, SEEK_END);
         write(fd2, SMTP_CMD_PERIOD, strlen(SMTP_CMD_PERIOD));
         close(fd2);
      }
   }


   if(print_message == 0 && result.spaminess >= cfg.spam_overall_limit && result.spaminess < 1.01)
      return 1;

   /* maildrop requires us to exit with 0 */
   return 0;
}
