/*
 * kospam.c, SJ
 */

#include <kospam.h>

extern char *optarg;
extern int optind;

char *configfile = CONFIG_FILE;
struct passwd *pwd;

void usage(){
   printf("\nusage: %s\n\n", PROGNAME);
   printf("    -c <config file>                  Config file to use if not the default\n");
   printf("    -d                                Fork to the background\n");
   printf("    -v                                Return the version\n");
   printf("    -V                                Return the version and some build parameters\n");

   exit(0);
}


void process_email(char *filename, MYSQL *conn, int size){
   struct timezone tz;
   struct timeval tv1, tv2;
   struct parser_state parser_state;
   struct counters counters;

   bzero(&counters, sizeof(counters));

   struct session_data sdata;

   init_session_data(&sdata);

   sdata.tot_len = size;

   snprintf(sdata.ttmpfile, sizeof(sdata.ttmpfile)-1, "%s", filename);

   // parse message

   struct Message m;

   gettimeofday(&tv1, &tz);
   parse_message(filename, &parser_state, &m);
   post_parse(&parser_state, &m, &cfg);
   gettimeofday(&tv2, &tz);
   sdata.__parsed = tvdiff(tv2, tv1);

   // TODO: If the email was bounced back from a remote server's MAILER-DAEMON then check our signo
   // if((strstr(sdata.mailfrom, "MAILER-DAEMON") || strstr(sdata.mailfrom, "<>")) && strlen(cfg->our_signo) > 3) sdata.need_signo_check = 1;

   if (cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "INFO: %s: hostname=%s, ip=%s", sdata.ttmpfile, parser_state.hostname, parser_state.ip);

   // TODO: virus check

   //char virusinfo[SMALLBUFSIZE];
   //sdata->rav = check_for_known_bad_attachments(sdata, &parser_state);
   //if(sdata->rav == AVIR_VIRUS) snprintf(virusinfo, sizeof(virusinfo)-1, "MARKED.AS.MALWARE");

   if (is_item_on_list(parser_state.ip, cfg.mynetwork, "") == 1) {
      syslog(LOG_PRIORITY, "%s: client ip (%s) on mynetwork", sdata.ttmpfile, parser_state.ip);
      sdata.mynetwork = 1;
   }

   if (parser_state.trapped) {
       counters.c_minefield++;
       gettimeofday(&tv1, &tz);
       store_minefield_ip(conn, parser_state.ip);
       gettimeofday(&tv2, &tz);
       sdata.__minefield = tvdiff(tv2, tv1);

       syslog(LOG_PRIORITY, "INFO: %s: we trapped %s on the blackhole", sdata.ttmpfile, parser_state.ip);
   }


   check_spam(&sdata, conn, &parser_state, &data, &cfg);

   char status[SMALLBUFSIZE];

   if (parser_state.training_request == 0) {
      write_history_to_sql(conn, &sdata,  &parser_state);

      // Update counters

      if(sdata.spaminess >= cfg.spam_overall_limit){
         sdata.status = S_SPAM;
         counters.c_spam++;
         snprintf(status, sizeof(status)-1, "SPAM");
      /*else if(sdata->rav == AVIR_VIRUS){
         counters.c_virus++;
         sdata->status = S_VIRUS;
         snprintf(status, sizeof(status)-1, "VIRUS (%s)", virusinfo);*/
      } else {
         sdata.status = S_HAM;
         counters.c_ham++;
         snprintf(status, sizeof(status)-1, "HAM");

         if(sdata.spaminess < cfg.spam_overall_limit && sdata.spaminess > cfg.possible_spam_limit) counters.c_possible_spam++;
         else if(sdata.spaminess < cfg.possible_spam_limit && sdata.spaminess > cfg.max_ham_spamicity) counters.c_unsure++;
      }

      // Modify message, and add our headers

      fix_message_file(&sdata, &cfg);

      // Move message to send dir

      char tmpbuf[SMALLBUFSIZE];
      snprintf(tmpbuf, sizeof(tmpbuf)-1, "%s/%s", SEND_DIR, sdata.ttmpfile);

      if (rename(filename, tmpbuf)) {
         syslog(LOG_PRIORITY, "ERROR: failed to rename %s to %s", filename, tmpbuf);
      }

   }
   else {
      snprintf(status, sizeof(status)-1, "TRAIN");
      unlink(filename);
   }

   clearhash(parser_state.token_hash);
   clearhash(parser_state.url);

   char delay[SMALLBUFSIZE];
   float total = sdata.__parsed+sdata.__av+sdata.__user+sdata.__policy+sdata.__minefield+sdata.__as+sdata.__training+sdata.__update;

   snprintf(delay, sizeof(delay)-1, "delay=%.2f, delays=%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f",
           total/1000000.0,
           sdata.__parsed/1000000.0,
           sdata.__av/1000000.0,
           sdata.__user/1000000.0,
           sdata.__policy/1000000.0,
           sdata.__minefield/1000000.0,
           sdata.__as/1000000.0,
           sdata.__training/1000000.0,
           sdata.__update/1000000.0);

   if(cfg.log_subject == 1) syslog(LOG_PRIORITY, "%s: subject=%s", filename, parser_state.b_subject);

   syslog(LOG_PRIORITY, "%s: from=%s, result=%s/%.4f, size=%d, attachments=%d, %s",
          filename, parser_state.envelope_from, status, sdata.spaminess, sdata.tot_len, m.n_attachments, delay);

   counters.c_rcvd++;

   update_counters(conn, &counters);
}


int process_dir(char *directory, MYSQL *conn){
   int tot_msgs=0;

   DIR *dir = opendir(directory);
   if(!dir){
      syslog(LOG_PRIORITY, "ERROR: cannot open directory: %s", directory);
      return tot_msgs;
   }

   struct dirent *de;

   while((de = readdir(dir))){
      if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

      char fname[SMALLBUFSIZE];

      snprintf(fname, sizeof(fname)-1, "%s/%s", directory, de->d_name);

      struct stat st;
      if(stat(fname, &st) == 0){
         // Move email to cfg.workdir (=current dir). This prevents any looping issue
         // if the email crashes the kospam child process and leaves temp files in the workdir.

         rename(fname, de->d_name);

         if(S_ISREG(st.st_mode)) {
            process_email(de->d_name, conn, st.st_size);
            tot_msgs++;
         }
      }
      else {
         syslog(LOG_PRIORITY, "ERROR: cannot stat: %s", fname);
      }
   }

   closedir(dir);

   return tot_msgs;
}


void child_main(struct child *ptr){
   char dir[TINYBUFSIZE];

   /* open directory, then process its files, then sleep 1 sec, and repeat */

   ptr->messages = 0;

   snprintf(dir, sizeof(dir)-1, "%d", ptr->serial);

   if(cfg.verbosity >= _LOG_DEBUG)
      syslog(LOG_PRIORITY, "child (pid: %d, serial: %d) started main() working on '%s'", getpid(), ptr->serial, dir);

   while(1){
      if(received_sighup == 1){
         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "child (pid: %d) caught HUP signal", getpid());
         break;
      }

      sig_block(SIGHUP);

      MYSQL *conn = open_database(&cfg);

      if (conn) {
         ptr->messages += process_dir(dir, conn);
         close_database(conn);

         sleep(1);
      }
      else {
         syslog(LOG_PRIORITY, "ERROR: cannot open database");
         sleep(10);
      }

      sig_unblock(SIGHUP);

      if(cfg.max_requests_per_child > 0 && ptr->messages >= cfg.max_requests_per_child){
         if(cfg.verbosity >= _LOG_DEBUG)
            syslog(LOG_PRIORITY, "child (pid: %d, serial: %d) served enough: %d", getpid(), ptr->serial, ptr->messages);
         break;
      }

   }

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "child decides to exit (pid: %d)", getpid());

   exit(0);
}


void fatal(char *s){
   syslog(LOG_PRIORITY, "%s", s);
   p_clean_exit();
}


void initialise_configuration(){
   struct stat st;

   if(stat(configfile, &st)) fatal("cannot read config file");

   cfg = read_config(configfile);

   if(cfg.number_of_worker_processes < 2)
      cfg.number_of_worker_processes = 2;

   if(cfg.number_of_worker_processes > MAXCHILDREN)
      cfg.number_of_worker_processes = MAXCHILDREN;

   if(strlen(cfg.username) > 1){
      pwd = getpwnam(cfg.username);
      if(!pwd) fatal(ERR_NON_EXISTENT_USER);
   }

   if(chdir(cfg.workdir)){
      syslog(LOG_PRIORITY, "workdir: *%s*", cfg.workdir);
      fatal(ERR_CHDIR);
   }

   setlocale(LC_MESSAGES, cfg.locale);
   setlocale(LC_CTYPE, cfg.locale);

   clearhash(data.mydomains);

   inithash(data.mydomains);

#ifdef HAVE_TRE
   zombie_init(&data, &cfg);
#endif

   syslog(LOG_PRIORITY, "reloaded config: %s", configfile);
}


int main(int argc, char **argv){
   int i, daemonise=0;
   struct stat st;

   while((i = getopt(argc, argv, "c:dvVh")) > 0){
      switch(i){

        case 'c' :
                   configfile = optarg;
                   break;

        case 'd' :
                   daemonise = 1;
                   break;

        case 'v' :
                   printf("%s-%s-%s\n", VERSION, COMMIT_ID, ARCH);
                   return 0;

        case 'V' :
                   printf("\n%s %s-%s, Janos SUTO\n\n%s\nMySQL client library version: %s\n",
                          PROGNAME, VERSION, COMMIT_ID, CONFIGURE_PARAMS, mysql_get_client_info());

                   return 0;

        case 'h' :
        default  :
                   usage();
      }
   }

   (void) openlog(PROGNAME, LOG_PID, LOG_MAIL);

   disable_coredump();

   initialise_configuration();

   set_signal_handler(SIGPIPE, SIG_IGN);

   if(drop_privileges(pwd)) fatal(ERR_SETUID);

   check_and_create_directories(&cfg);

   if(stat(cfg.pidfile, &st) == 0) {
       syslog(LOG_PRIORITY, "WARN: pidfile %s exists, unclean shutdown?", cfg.pidfile);
       unlink(cfg.pidfile);
   }

   syslog(LOG_PRIORITY, "%s %s-%s starting", PROGNAME, VERSION, COMMIT_ID);

#if HAVE_DAEMON
   if(daemonise == 1 && daemon(1, 0) == -1) fatal(ERR_DAEMON);
#endif

   write_pid_file(cfg.pidfile);

   manage_partitions(&cfg);

   time_t last_partition_maintenance = time(NULL);

   child_pool_create(cfg.number_of_worker_processes);

   set_signal_handler(SIGCHLD, takesig);
   set_signal_handler(SIGTERM, takesig);
   set_signal_handler(SIGHUP, takesig);


   for(;;){
      sleep(10);

      time_t now = time(NULL);
      if (now - last_partition_maintenance >= 3600) {
         manage_partitions(&cfg);
         last_partition_maintenance = now;
      }
   }

   p_clean_exit();

   return 0;
}
