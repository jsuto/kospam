/*
 * kospam.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <locale.h>
#include <errno.h>
#include <kospam.h>

#define PROGNAME "kospam"

extern char *optarg;
extern int optind;

int quit = 0;
int received_sighup = 0;
char *configfile = CONFIG_FILE;
struct __config cfg;
struct __data data;
struct passwd *pwd;

struct child children[MAXCHILDREN];


void usage(){
   printf("\nusage: %s\n\n", PROGNAME);
   printf("    -c <config file>                  Config file to use if not the default\n");
   printf("    -d                                Fork to the background\n");
   printf("    -v                                Return the version and build number\n");
   printf("    -V                                Return the version and some build parameters\n");

   exit(0);
}


static void takesig(int sig){
   int i, status;
   pid_t pid;

   switch(sig){
        case SIGHUP:
                initialise_configuration();
                kill_children(SIGHUP, "SIGHUP");
                break;

        case SIGTERM:
        case SIGKILL:
                quit = 1;
                p_clean_exit();
                break;

        case SIGCHLD:
                while((pid = waitpid (-1, &status, WNOHANG)) > 0){

                   if(quit == 0){
                      i = search_slot_by_pid(pid);
                      if(i >= 0){
                         children[i].serial = i;
                         children[i].status = READY;
                         children[i].pid = child_make(&children[i]);
                      }
                      else syslog(LOG_PRIORITY, "ERROR: couldn't find slot for pid %d", pid);

                   }
                }
                break;
   }

   return;
}


void child_sighup_handler(int sig){
   if(sig == SIGHUP){
      received_sighup = 1;
   }
}


/*int perform_checks(struct session_data *sdata, struct data *data, struct parser_state *parser_state, struct __config *cfg){
   struct timezone tz;
   struct timeval tv1, tv2;
   int rc=ERR;

   gettimeofday(&tv1, &tz);
   int rc = process_message(sdata, parser_state, data, cfg);
   unlink(parser_state->message_id_hash);

   gettimeofday(&tv2, &tz);
   sdata->__process_message = tvdiff(tv2, tv1);

   return rc;
}*/


int process_email(char *filename, struct session_data *sdata, int size){
   //char tmpbuf[SMALLBUFSIZE];
   //char *status=S_STATUS_UNDEF;
   //char *p;
   struct timezone tz;
   struct timeval tv1, tv2;
   struct __state parser_state;
   struct __counters counters;

   bzero(&counters, sizeof(counters));

   init_session_data(sdata, &cfg);

   sdata->tot_len = size;

   snprintf(sdata->filename, SMALLBUFSIZE-1, "%s", filename);

   // parse message

   gettimeofday(&tv1, &tz);
   parser_state = parse_message(sdata, 1, &cfg);
   post_parse(&parser_state);
   gettimeofday(&tv2, &tz);
   sdata->__parsed = tvdiff(tv2, tv1);

   // TODO: virus check

   char virusinfo[SMALLBUFSIZE];

   sdata->rav = check_for_known_bad_attachments(sdata, &parser_state);
   if(sdata->rav == AVIR_VIRUS) snprintf(virusinfo, sizeof(virusinfo)-1, "MARKED.AS.MALWARE");

   if (is_item_on_list(sdata->ip, cfg.mynetwork, "") == 1) {
      syslog(LOG_PRIORITY, "%s: client ip (%s) on mynetwork", sdata->ttmpfile, sdata->ip);
      sdata->mynetwork = 1;
   }

   struct __config my_cfg;
   memcpy(&my_cfg, &cfg, sizeof(struct __config));

   char recipient[SMALLBUFSIZE];

   snprintf(recipient, sizeof(recipient)-1, "%s", sdata->rcptto[0]);
   extract_verp_address(recipient);

   check_spam(sdata, &parser_state, &data, sdata->fromemail, recipient, &cfg, &my_cfg);

   int rc = ERR;
   //int rc = perform_checks(sdata, &data, &parser_state, &cfg);
   //unlink(sdata->tmpframe);

   if(rc != ERR) unlink(sdata->filename);

   //update_counters(sdata, &counters);

   char delay[SMALLBUFSIZE];
   snprintf(delay, sizeof(delay)-1, "delay=%.2f, delays=%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f",
           (sdata->__acquire+sdata->__parsed+sdata->__av+sdata->__user+sdata->__policy+sdata->__minefield+sdata->__as+sdata->__training+sdata->__update+sdata->__store+sdata->__inject)/1000000.0,
           sdata->__acquire/1000000.0,
           sdata->__parsed/1000000.0,
           sdata->__av/1000000.0,
           sdata->__user/1000000.0,
           sdata->__policy/1000000.0,
           sdata->__minefield/1000000.0,
           sdata->__as/1000000.0,
           sdata->__training/1000000.0,
           sdata->__update/1000000.0,
           sdata->__store/1000000.0,
           sdata->__inject/1000000.0);


   char tmpbuf[SMALLBUFSIZE];

   if(sdata->rav == AVIR_VIRUS){
      counters.c_virus++;
      sdata->status = S_VIRUS;
      snprintf(tmpbuf, sizeof(tmpbuf)-1, "VIRUS (%s)", virusinfo);
   } else if(sdata->spaminess >= my_cfg.spam_overall_limit){
      sdata->status = S_SPAM;
      counters.c_spam++;
      snprintf(tmpbuf, sizeof(tmpbuf)-1, "SPAM");
   } else {
      sdata->status = S_HAM;
      counters.c_ham++;
      snprintf(tmpbuf, sizeof(tmpbuf)-1, "HAM");

      if(sdata->spaminess < my_cfg.spam_overall_limit && sdata->spaminess > my_cfg.possible_spam_limit) counters.c_possible_spam++;
      else if(sdata->spaminess < my_cfg.possible_spam_limit && sdata->spaminess > my_cfg.max_ham_spamicity) counters.c_unsure++;
   }

   if(cfg.log_subject == 1) syslog(LOG_PRIORITY, "%s: subject=%s", sdata->ttmpfile, parser_state.b_subject);
   syslog(LOG_PRIORITY, "%s: from=%s, result=%s/%.4f, size=%d, attachments=%d, %s", sdata->ttmpfile, sdata->fromemail, tmpbuf, sdata->spaminess, sdata->tot_len, parser_state.n_attachments, delay);

   /*if(sdata->training_request == 0){
      if(write_history(sdata, &state, inject_resp, &my_cfg) != OK) syslog(LOG_PRIORITY, "%s: error: failed inserting to history", sdata->ttmpfile);
   }*/

   unlink(sdata->ttmpfile);

   clearhash(parser_state.token_hash);
   clearhash(parser_state.url);

   return rc;
}


int process_dir(char *directory, struct session_data *sdata){
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

         if(S_ISREG(st.st_mode) && process_email(de->d_name, sdata, st.st_size) != ERR){
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
   struct session_data sdata;
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

      if(open_database(&sdata, &cfg) == OK){
         ptr->messages += process_dir(dir, &sdata);
         close_database(&sdata);

         sleep(1);
      }
      else {
         syslog(LOG_PRIORITY, "ERROR: cannot open database");
         sleep(10);
      }

      sig_unblock(SIGHUP);

      if(cfg.max_requests_per_child > 0 && ptr->messages >= cfg.max_requests_per_child){
         if(cfg.verbosity >= _LOG_DEBUG)
            syslog(LOG_PRIORITY, "child (pid: %d, serial: %d) served enough: %d", getpid(), ptr->messages, ptr->serial);
         break;
      }

   }

   //if(cfg.memcached_enable == 1) memcached_shutdown(&(data.memc));

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "child decides to exit (pid: %d)", getpid());

   //post_process(&data, &cfg);

   exit(0);
}


pid_t child_make(struct child *ptr){
   pid_t pid;

   if((pid = fork()) > 0) return pid;

   if(pid == -1) return -1;

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "forked a child (pid: %d)", getpid());

   /* reset signals */

   set_signal_handler(SIGCHLD, SIG_DFL);
   set_signal_handler(SIGTERM, SIG_DFL);
   set_signal_handler(SIGHUP, child_sighup_handler);

   child_main(ptr);

   return -1;
}



int child_pool_create(){
   int i;

   for(i=0; i<MAXCHILDREN; i++){
      children[i].pid = 0;
      children[i].messages = 0;
      children[i].status = UNDEF;
      children[i].serial = -1;
   }

   for(i=0; i<cfg.number_of_worker_processes; i++){
      children[i].status = READY;
      children[i].serial = i;
      children[i].pid = child_make(&children[i]);

      if(children[i].pid == -1){
         syslog(LOG_PRIORITY, "error: failed to fork a child");
         p_clean_exit();
      }
   }

   return 0;
}


int search_slot_by_pid(pid_t pid){
   int i;

   for(i=0; i<MAXCHILDREN; i++){
      if(children[i].pid == pid) return i;
   }

   return -1;
}


void kill_children(int sig, char *sig_text){
   int i;

   for(i=0; i<MAXCHILDREN; i++){
      if(children[i].status != UNDEF && children[i].pid > 1){
         if(cfg.verbosity >= _LOG_DEBUG)
            syslog(LOG_PRIORITY, "sending signal %s to child (pid: %d)", sig_text, children[i].pid);

         kill(children[i].pid, sig);
      }
   }
}


void p_clean_exit(){
   kill_children(SIGTERM, "SIGTERM");

   clearhash(data.mydomains);

#ifdef HAVE_TRE
   zombie_free(&data);
#endif

   syslog(LOG_PRIORITY, "%s has been terminated", PROGNAME);

   unlink(cfg.pidfile);

   closelog();

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

   //if(cfg.memcached_enable == 1) memcached_init(&(data.memc), cfg.memcached_servers, 11211);
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

   if(stat(cfg.pidfile, &st) == 0) fatal("pidfile exists! Unclean shutdown?");

   syslog(LOG_PRIORITY, "%s %s-%s starting", PROGNAME, VERSION, COMMIT_ID);

#if HAVE_DAEMON
   if(daemonise == 1 && daemon(1, 0) == -1) fatal(ERR_DAEMON);
#endif

   write_pid_file(cfg.pidfile);

   child_pool_create();

   set_signal_handler(SIGCHLD, takesig);
   set_signal_handler(SIGTERM, takesig);
   set_signal_handler(SIGHUP, takesig);


   for(;;){ sleep(1); }

   p_clean_exit();

   return 0;
}
