/*
 * spamdrop.c, SJ
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
#include <libgen.h>

extern char *optarg;
extern int optind;

int deliver_message=0;


void query_unix_account(struct session_data *sdata, char *username){
   struct passwd *pwd;

   if(username){
      if((pwd = getpwnam(username)) != (struct passwd *)NULL) {
         sdata->uid = pwd->pw_uid;
         snprintf(sdata->name, SMALLBUFSIZE-1, "%s", username);
      }
   }
   else {
      if(sdata->uid == -1) sdata->uid = getuid();
      if((pwd = getpwuid(sdata->uid)) != (struct passwd *)NULL) {
         snprintf(sdata->name, SMALLBUFSIZE-1, "%s", pwd->pw_name);
      }
   }

}


int openDatabase(struct session_data *sdata, struct __config *cfg){
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


void closeDatabase(struct session_data *sdata){

#ifdef HAVE_MYSQL
   mysql_close(&(sdata->mysql));
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(sdata->db);
#endif
#ifdef HAVE_MYDB
   close_mydb(sdata->mhash);
#endif
}


/* print message to stdout/pipe */

int print_message_stdout(struct session_data *sdata, char *clapf_info, float spaminess, struct __config *cfg){
   FILE *f, *ofile=stdout;
   int put_subject_spam_prefix=0, sent_subject_spam_prefix=0, is_header=1;
   char buf[MAXBUFSIZE];


   f = fopen(sdata->ttmpfile, "r");
   if(!f){
      syslog(LOG_PRIORITY, "cannot read: %s", sdata->ttmpfile);
      return EX_TEMPFAIL;
   }

   if(spaminess >= cfg->spam_overall_limit && spaminess < 1.01 && strlen(cfg->spam_subject_prefix) > 1) put_subject_spam_prefix = 1;


   if(deliver_message == 1){
      snprintf(buf, MAXBUFSIZE-1, "%s %s %s", cfg->delivery_agent, sdata->mailfrom, sdata->rcptto[0]);
      ofile = popen(buf, "w");
   }

   if(clapf_info) fprintf(ofile, "%s", clapf_info);


   while(fgets(buf, MAXBUFSIZE-1, f)){

      /* tag the Subject line if we have to, 2007.08.21, SJ */

      if(is_header == 1 && put_subject_spam_prefix == 1 && strncmp(buf, "Subject:", 8) == 0 && !strstr(buf, cfg->spam_subject_prefix)){
         fprintf(ofile, "Subject: ");
         fprintf(ofile, "%s", cfg->spam_subject_prefix);
         fprintf(ofile, "%s", &buf[9]);
         sent_subject_spam_prefix = 1;

         continue;
      }

      if(is_header == 1 && (buf[0] == '\n' || buf[0] == '\r')){
         is_header = 0;

         /* if we did not find a Subject line, add one - if we have to */

         if(sent_subject_spam_prefix == 0 && put_subject_spam_prefix == 1 && spaminess >= cfg->spam_overall_limit && spaminess < 1.01)
            fprintf(ofile, "Subject: %s%s", cfg->spam_subject_prefix, CRLF);

      }

      if(is_header == 0 || strncmp(buf, cfg->clapf_header_field, strlen(cfg->clapf_header_field)))
         fprintf(ofile, "%s", buf);
   }

   fclose(stdout);

   return 0;
}



int main(int argc, char **argv, char **envp){
   int i, n, fd, rc=0, rounds=1, debug=0, quiet=0, compact=0;
   int print_message=1, no_chdir=0;
   int is_spam=0, train_as_ham=0, train_as_spam=0, blackhole_request=0, training_request=0;
   int train_mode=T_TOE;
   int u=-1;
   char buf[MAXBUFSIZE], trainbuf[SMALLBUFSIZE], ID[RND_STR_LEN+1], whitelistbuf[SMALLBUFSIZE], clapf_info[MAXBUFSIZE];
   char *configfile=CONFIG_FILE, *from=NULL, *recipient=NULL;
   char *p, path[SMALLBUFSIZE];
   char *username=NULL;
   struct session_data sdata;
   struct timezone tz;
   struct timeval tv_start, tv_stop;
   struct _state state;
   struct __data data;
   struct __config cfg;
   float spaminess=DEFAULT_SPAMICITY;
#ifdef HAVE_LANG_DETECT
   char *lang="unknown";
#endif
#ifdef HAVE_SPAMDROP_HELPER
   struct stat st;
   char envvar[SMALLBUFSIZE];
   char *eeenv[] = { NULL, (char *) 0 };
#endif
#ifdef HAVE_SPAMSUM
   char *sum, spamsum_buf[SMALLBUFSIZE];
   unsigned int flags=0, spamsum_score=0;
#endif


   while((i = getopt(argc, argv, "c:U:f:r:u:SHDCQdhqo?")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'U' :
                    u = atoi(optarg);
                    break;

         case 'u' :
                    username = optarg;
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

         case 'q' :
                    quiet++;
                    if(quiet > 2) quiet = 2;
                    break;

         case 'S' :
                    train_as_spam = 1;
                    print_message = 0;
                    break;

         case 'H' :
                    train_as_ham = 1;
                    print_message = 0;
                    break;

         case 'Q' :
                    print_message = 0;
                    break;

         case 'D' :
                    debug = 1;
                    break;

         case 'C' :
                    no_chdir = 1;
                    break;

         case 'o' :
                    compact = 1;
                    break;

         case 'h' :
         case '?' :
                    printf("%s\n", SPAMDROPUSAGE);
                    break;


         default  : 
                    break;
       }
   }


   (void) openlog(basename(argv[0]), LOG_PID, LOG_MAIL);

   if(strstr(argv[0], "blackhole")) blackhole_request = 1;

   if(train_as_spam == 1 && train_as_ham == 1){
      fprintf(stderr, "%s\n", ERR_TRAIN_AS_HAMSPAM);
      return 0;
   }


   cfg = read_config(configfile);

   if(debug == 1){
      print_message = 0;
      cfg.verbosity = 0;
      cfg.debug = 1;
   }

   if(quiet > 0) cfg.verbosity = 0;

   setlocale(LC_MESSAGES, cfg.locale);
   setlocale(LC_CTYPE, cfg.locale);


   /*
    * read the 'FROM' environment variable if you have
    * not specified the -f cmdline switch
    */

   if(!from) from = getenv("FROM");


   initSessionData(&sdata);

#ifdef HAVE_MEMCACHED
   memcached_init(&(data.memc), cfg.memcached_servers, 11211);
#endif

   if(recipient) snprintf(sdata.rcptto[0], SMALLBUFSIZE-1, "%s", recipient);

   memset(trainbuf, 0, SMALLBUFSIZE);
   memset(clapf_info, 0, MAXBUFSIZE);

#ifdef HAVE_TRE
   initialiseZombieList(&data, &cfg);
#endif

   /* spamdrop is now setuid to clapf:clapf, so we can always get to the workdir */
   if(no_chdir == 0) i = chdir(cfg.workdir);

   /* read message from standard input */


   fd = open(sdata.ttmpfile, O_CREAT|O_EXCL|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
   if(fd == -1){
      close(fd);
      syslog(LOG_PRIORITY, "cannot open: %s", sdata.ttmpfile);
      return EX_TEMPFAIL;
   }

   while((n = read(0, buf, MAXBUFSIZE)) > 0){
      sdata.tot_len += write(fd, buf, n);
   }


   /* make sure we had a successful read */

   if(fsync(fd)){
      syslog(LOG_PRIORITY, "failed writing data: %s", sdata.ttmpfile);
      return EX_TEMPFAIL;
   }

   close(fd);


   gettimeofday(&tv_start, &tz);


   /* do antivirus check if we have to */

#ifdef HAVE_ANTIVIRUS
#ifndef HAVE_LIBCLAMAV
   char virusinfo[SMALLBUFSIZE];

   if(do_av_check(&sdata, recipient, from, &virusinfo[0], &data, &cfg) == AVIR_VIRUS){
      syslog(LOG_PRIORITY, "%s: dropping infected message", sdata.ttmpfile);
      unlink(sdata.ttmpfile);
      return 0;
   }
#endif
#endif


   /* 
    * check whether this is a training request with user+spam@... or user+ham@...
    */

   if(recipient && (strcasestr(recipient, "+ham@") || strcasestr(recipient, "+spam@"))){
      training_request = 1;
   }
   else {
      FILE *f;
      f = fopen(sdata.ttmpfile, "r");
      if(f){
         while(fgets(trainbuf, SMALLBUFSIZE-1, f)){
            if(strncmp(trainbuf, "To:", 3) == 0 && (strcasestr(trainbuf, "+ham@") || strcasestr(trainbuf, "+spam@")) ){
               trimBuffer(trainbuf);
               training_request = 1;
               break;
            }

            if(trainbuf[0] == '\r' || trainbuf[0] == '\n') break;
         }
         fclose(f);
      }

   }

   /* skip spamicity check if message is too long, and we are not debugging nor training */

   if(print_message == 1 && cfg.max_message_size_to_filter > 0 && sdata.tot_len > cfg.max_message_size_to_filter && cfg.debug == 0 && training_request == 0 && train_as_ham == 0 && train_as_spam == 0){

      gettimeofday(&tv_stop, &tz);
      memset(trainbuf, 0, SMALLBUFSIZE);
      goto ENDE_SPAMDROP;
   }



   /* we must have a FROM address for training */

   if(training_request == 1 && from == NULL){
      syslog(LOG_PRIORITY, "%s: no FROM address detected for training", sdata.ttmpfile);
      unlink(sdata.ttmpfile);
      return 0;
   }


   /* fix username and uid */

   if(recipient == NULL && from == NULL) query_unix_account(&sdata, username != (char *)NULL ? username : getenv("LOGNAME"));

   if(sdata.uid == -1){
      if(u > 0) sdata.uid = u;
      query_unix_account(&sdata, (char *)NULL);
   }

   sdata.gid = sdata.uid;


   /* fix database path if we need it */

#ifdef HAVE_SQLITE3
   if(strlen(cfg.sqlite3) < 4)
      snprintf(cfg.sqlite3, MAXVAL-1, "%s/%s/%c/%s/%s", cfg.chrootdir, USER_DATA_DIR, sdata.name[0], sdata.name, PER_USER_SQLITE3_DB_FILE);
#endif
#ifdef HAVE_MYDB
   if(strlen(cfg.mydbfile) < 4)
      snprintf(cfg.mydbfile, MAXVAL-1, "%s/%s/%c/%s/%s", cfg.chrootdir, USER_DATA_DIR, sdata.name[0], sdata.name, MYDB_FILE);
#endif

#ifdef HAVE_SPAMDROP_HELPER
   snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s", cfg.chrootdir, cfg.queuedir, sdata.name[0], sdata.name);

   if(stat(buf, &st) != 0){
      if(stat(SPAMDROP_HELPER_PROGRAM, &st) != 0){
         syslog(LOG_PRIORITY, "%s: missing spamdrop helper script: %s, for user: %s", sdata.ttmpfile, SPAMDROP_HELPER_PROGRAM, sdata.name);
         return EX_TEMPFAIL;
      }

      syslog(LOG_PRIORITY, "%s: running spamdrop helper script: %s, for user: %s", sdata.ttmpfile, SPAMDROP_HELPER_PROGRAM, sdata.name);

      snprintf(envvar, SMALLBUFSIZE-1, "YOURUSERNAME=%s", sdata.name);
      putenv(envvar);
      eeenv[0] = &envvar[0];

      execl(SPAMDROP_HELPER_PROGRAM, envvar, (char*)0);

      if(stat(buf, &st) != 0){
         syslog(LOG_PRIORITY, "%s: missing user directory: %s", sdata.ttmpfile, buf);
         return EX_TEMPFAIL;
      }
   }
#endif



   if(openDatabase(&sdata, &cfg) == 0){
      rc = print_message_stdout(&sdata, NULL, spaminess, &cfg);
      unlink(sdata.ttmpfile);
      if(rc)
         return EX_TEMPFAIL;
      else
         return 0;
   }


#ifdef HAVE_USERS
   if(recipient) getUserdataFromEmail(&sdata, recipient, &cfg);
#endif

   /*
    * handle training requests
    */

   if(training_request == 1 && (recipient || strlen(trainbuf) > 3)){
      rounds = MAX_ITERATIVE_TRAIN_LOOPS;
      is_spam = 0;

      if(recipient && strcasestr(recipient, "+spam@")) is_spam = 1;
      if(strlen(trainbuf) > 3 && strcasestr(trainbuf, "+spam@")) is_spam = 1;


      /* determine the queue file from the message */
      train_mode = extract_id_from_message(sdata.ttmpfile, cfg.clapf_header_field, ID);

      if(!recipient) recipient = trainbuf;

      syslog(LOG_PRIORITY, "%s: training request for %s (username: %s, uid: %ld, gid: %ld), found id: %s", sdata.ttmpfile, recipient, sdata.name, sdata.uid, sdata.gid, ID);

      if(strlen(ID) < 5){
         syslog(LOG_PRIORITY, "%s: not found a valid message id (%s)", sdata.ttmpfile, ID);
         return 0;
      }

      /* determine the path of the original file */

      p = &path[0];
      get_queue_path(&sdata, &p, &cfg);


      if(is_spam == 1){
         snprintf(buf, MAXBUFSIZE-1, "%s/h.%s", path, ID);
      }
      else {
         snprintf(buf, MAXBUFSIZE-1, "%s/s.%s", path, ID);
      }


      snprintf(sdata.ttmpfile, SMALLBUFSIZE-1, "%s", buf);
      state = parseMessage(&sdata, &cfg);

      /* is it a TUM trained message? */

      if(state.train_mode == T_TUM)
         train_mode=T_TUM;

      /* ... then train with the message */

      trainMessage(&sdata, &state, rounds, is_spam, train_mode, &cfg);

      closeDatabase(&sdata); freeState(&state);
      unlink(sdata.ttmpfile);

      return 0;
   }


   memset(trainbuf, 0, SMALLBUFSIZE);

   if(cfg.verbosity >= _LOG_DEBUG && debug == 0) syslog(LOG_PRIORITY, "%s: username: %s, uid: %ld, gid: %ld", sdata.ttmpfile, sdata.name, sdata.uid, sdata.gid);


   /* parse message */
   state = parseMessage(&sdata, &cfg);

   if(cfg.debug == 1) {
      printf("attachment list:\n");
      for(i=0; i<state.n_attachments; i++){
         printf("   part-%d: filename:%s, type:%s, len:%d\n", i, state.attachments[i].filename, state.attachments[i].type, state.attachments[i].size);
      }
      printf("\n");
   }

   #ifdef HAVE_TRE
      checkZombieSender(&sdata, &data, &state, &cfg);
   #endif

   /*
    * if this is a training request from the command line
    */

   if(train_as_ham == 1 || train_as_spam == 1){
      if(train_as_spam == 1) is_spam = 1;
      else is_spam = 0;

      /* is it a TUM trained message? */

      if(state.train_mode == T_TUM)
         train_mode=T_TUM;

   #ifdef HAVE_USERS

      /*
       * cmdline training (as spam in these examples) can be done several ways:
       *
       * spamdrop -S -f email@address < message
       * FROM=email@address spamdrop -S < message
       * spamdrop -S -U gid < message
       */

      if(from) getUserdataFromEmail(&sdata, from, &cfg);
      else if(u >= 0){
         sdata.uid = u;
         sdata.gid = sdata.uid;
      }
   #endif

      trainMessage(&sdata, &state, rounds, is_spam, train_mode, &cfg);

      closeDatabase(&sdata); freeState(&state);
      unlink(sdata.ttmpfile);

      return 0;
   }


   /*
    * or just calculate spamicity
    */

   else {

      /* is this a bounce message? */

      if(from && (strcasecmp(from, "MAILER-DAEMON") == 0 || strcmp(from, "<>") == 0) && strlen(cfg.our_signo) > 3){
         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: from: %s, we should really see our signo", sdata.ttmpfile, from);
         sdata.need_signo_check = 1;
      }


      /* whitelist check first */

   #ifdef HAVE_WHITELIST
      getUsersWBL(&sdata, &data, &cfg);

      if(isEmailAddressOnList(sdata.whitelist, sdata.ttmpfile, from, &cfg) == 1){
         syslog(LOG_PRIORITY, "%s: sender (%s) found on whitelist", sdata.ttmpfile, from);
         snprintf(whitelistbuf, SMALLBUFSIZE-1, "%sFound on whitelist%s", cfg.clapf_header_field, CRLF);
         strncat(clapf_info, whitelistbuf, MAXBUFSIZE-1);
         is_spam = 0;
         goto ENDE_SPAMDROP;
      }
   #endif

      /* then give blacklist a try */

   #ifdef HAVE_BLACKLIST
      if(isEmailAddressOnList(sdata.blacklist, sdata.ttmpfile, from, &cfg) == 1){
         syslog(LOG_PRIORITY, "%s: sender (%s) found on blacklist", sdata.ttmpfile, from);
         closeDatabase(&sdata); freeState(&state);
         unlink(sdata.ttmpfile);
         return 0;
      }
   #endif


      /* query spaminess */

      spaminess = bayes_file(&sdata, &state, &cfg);

      /* update tokens unless we are in debug mode */

      if(cfg.update_tokens == 1 && debug == 0){
      #ifdef HAVE_MYDB
         update_tokens(cfg.mydbfile, sdata.mhash, state.token_hash);
      #else
         updateTokenTimestamps(&sdata, state.token_hash);
      #endif
      }


   #ifdef HAVE_TRE
      if(sdata.tre == '+'){
         if(cfg.message_from_a_zombie == 1) spaminess = 0.99;
      }
   #endif

   #ifdef HAVE_LANG_DETECT
      lang = check_lang(state.token_hash);

      if(quiet < 2){
         strncat(clapf_info, cfg.clapf_header_field, MAXBUFSIZE-1);
         strncat(clapf_info, lang, MAXBUFSIZE-1);
         strncat(clapf_info, CRLF, MAXBUFSIZE-1);
      }

      if(cfg.debug == 1) printf("lang detected: %s\n", lang);
   #endif

   #ifdef HAVE_SPAMSUM
      /* if we are uncertain, try the spamsum module, 2008.04.28, SJ */

      if(spaminess > cfg.max_ham_spamicity && spaminess < cfg.spam_overall_limit){
         flags |= FLAG_IGNORE_HEADERS;
         sum = spamsum_file(sdata.ttmpfile, flags, 0);
         if(sum){
            spamsum_score = spamsum_match_db(cfg.sig_db, sum, 55);
            if(spamsum_score >= 50) spaminess = 0.9988;

            if(quiet < 2){
               snprintf(spamsum_buf, SMALLBUFSIZE-1, "%sspamsum=%d%s", cfg.clapf_header_field, spamsum_score, CRLF);
               strncat(clapf_info, spamsum_buf, MAXBUFSIZE-1);
            }

            free(sum);
         }
      }
   #endif

      if(sdata.need_signo_check == 1){
         if(!state.found_our_signo){
            syslog(LOG_PRIORITY, "%s: looks like a bounce, but our signo is missing", sdata.ttmpfile);
         }
         else {
            syslog(LOG_PRIORITY, "%s: found our signo, this should be a real bounce message", sdata.ttmpfile);
            spaminess = DEFAULT_SPAMICITY;
         }
      }


      if(spaminess >= cfg.spam_overall_limit)
         is_spam = 1;
      else
         is_spam = 0;



      /* don't TUM train if this is a blackhole message or we are just spamtest'ing */

      if(
         (blackhole_request == 0 && debug == 0 && cfg.training_mode == T_TUM && ( (spaminess >= cfg.spam_overall_limit && spaminess < 0.99) || (spaminess < cfg.max_ham_spamicity && spaminess > 0.1) )) ||
         (cfg.initial_1000_learning == 1 && (sdata.Nham < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED || sdata.Nspam < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED))
        )
      {

         if(cfg.verbosity > 0){
            if(is_spam == 1)
               syslog(LOG_PRIORITY, "%s: TUM training a spam", sdata.ttmpfile);
            else
               syslog(LOG_PRIORITY, "%s: TUM training a ham", sdata.ttmpfile);
         }
         snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM%s", cfg.clapf_header_field, CRLF);

         trainMessage(&sdata, &state, 1, is_spam, train_mode, &cfg);
      }

   }




   /* if this is a blackhole request and spaminess < 0.99, then learn the message in an iterative loop */


   if(blackhole_request == 1){
      if(spaminess < 0.99){
         rounds = MAX_ITERATIVE_TRAIN_LOOPS;

         if(cfg.verbosity > 0) syslog(LOG_PRIORITY, "%s: training on a blackhole message", sdata.ttmpfile);
         snprintf(trainbuf, SMALLBUFSIZE-1, "%sTUM on blackhole%s", cfg.clapf_header_field, CRLF);

         sdata.uid = sdata.gid = 0;
         trainMessage(&sdata, &state, rounds, 1, T_TOE, &cfg);
      }
   }


   gettimeofday(&tv_stop, &tz);

   if(cfg.debug == 1){
      printf("spaminess: %.4f in %ld [ms]\n", spaminess, tvdiff(tv_stop, tv_start)/1000);
      printf("%ld %ld\n", state.c_shit, state.l_shit);
      printf("state.ip/host: %s/%s\n", state.ip, state.hostname);
      printf("number of tokens: %ld/%ld/%ld\n", state.n_token, state.n_chain_token, state.n_body_token);
   }


ENDE_SPAMDROP:

   if(cfg.debug == 0 && cfg.verbosity > 0) syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, spaminess, sdata.tot_len, tvdiff(tv_stop, tv_start)/1000);


   /*
    * print message to stdout/pipe
    */

   if(print_message == 1){

      if(quiet < 2){
         if(compact == 1)
            snprintf(buf, MAXBUFSIZE-1, "%s%s%s%s%s%ld ms%s",
                 cfg.clapf_header_field, sdata.ttmpfile, CRLF, trainbuf, cfg.clapf_header_field, tvdiff(tv_stop, tv_start)/1000, CRLF);
         else
            snprintf(buf, MAXBUFSIZE-1, "%s%s%s%s%.4f%s%s%s%ld ms%s",
                 cfg.clapf_header_field, sdata.ttmpfile, CRLF, cfg.clapf_header_field, spaminess, CRLF, trainbuf, cfg.clapf_header_field, tvdiff(tv_stop, tv_start)/1000, CRLF);


         strncat(clapf_info, buf, MAXBUFSIZE-1);
      }

      if(sdata.statistically_whitelisted == 1){
         snprintf(buf, MAXBUFSIZE-1, "%sstatistically whitelisted%s", cfg.clapf_header_field, CRLF);
         strncat(clapf_info, buf, MAXBUFSIZE-1);
      }

      if(spaminess > 0.9999){
         strncat(clapf_info, cfg.clapf_header_field, MAXBUFSIZE-1);
         strncat(clapf_info, MSG_ABSOLUTELY_SPAM, MAXBUFSIZE-1);
         strncat(clapf_info, CRLF, MAXBUFSIZE-1);
      }

      if(compact == 1){
         if(spaminess >= cfg.spam_overall_limit && spaminess < 1.01)
            snprintf(buf, MAXBUFSIZE-1, "%sYes, %.4f%s", cfg.clapf_header_field, spaminess, CRLF);
         else
            snprintf(buf, MAXBUFSIZE-1, "%sNo, %.4f%s", cfg.clapf_header_field, spaminess, CRLF);

         strncat(clapf_info, buf, MAXBUFSIZE-1);
      }
      else if(spaminess >= cfg.spam_overall_limit && spaminess < 1.01){
         snprintf(buf, MAXBUFSIZE-1, "%sYes%s", cfg.clapf_header_field, CRLF);
         strncat(clapf_info, buf, MAXBUFSIZE-1);
      }

   #ifdef MY_TEST
      char rblbuf[SMALLBUFSIZE];

      memset(rblbuf, 0, SMALLBUFSIZE);
      reverse_ipv4_addr(state.ip);
      if(isIPv4AddressOnRBL(state.ip, "zen.spamhaus.org") == 1)
         snprintf(rblbuf, SMALLBUFSIZE-1, "%sZEN=1%s", cfg.clapf_header_field, CRLF);
      else
         snprintf(rblbuf, SMALLBUFSIZE-1, "%sZEN=0%s", cfg.clapf_header_field, CRLF);

      strncat(clapf_info, rblbuf, MAXBUFSIZE-1);
   #endif

      rc = print_message_stdout(&sdata, clapf_info, spaminess, &cfg);
      if(rc) return rc;
   }




   if(cfg.debug == 0 && cfg.verbosity > 0){
      snprintf(buf, MAXBUFSIZE-1, "%ld", sdata.uid);

      if(is_spam == 1)
         syslog(LOG_PRIORITY, "%s: %s got SPAM", sdata.ttmpfile, buf);
      else
         syslog(LOG_PRIORITY, "%s: %s got HAM", sdata.ttmpfile, buf);
   }


   closeDatabase(&sdata); freeState(&state);


   /* save email for later retraining and/or spam quarantine */

#ifdef HAVE_STORE
   if( (sdata.tot_len <= cfg.max_message_size_to_filter || cfg.max_message_size_to_filter == 0) && blackhole_request == 0 && debug == 0){

      /* add trailing dot to the file, 2010.02.18, SJ */

      if(debug == 0){
         fd = open(sdata.ttmpfile, O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
         if(fd != -1){
            lseek(fd, 0, SEEK_END);
            i = write(fd, SMTP_CMD_PERIOD, strlen(SMTP_CMD_PERIOD));
            close(fd);
         }
      }

      saveMessageToQueue(&sdata, spaminess, &cfg);
   }
#endif

   unlink(sdata.ttmpfile);



   if(print_message == 0 && spaminess >= cfg.spam_overall_limit && spaminess < 1.01)
      return 1;

   /* maildrop requires us to exit with 0 */
   return 0;
}
