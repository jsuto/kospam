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


void p_clean_exit();
void initialise_configuration();
int search_slot_by_pid(pid_t pid);
pid_t child_make(struct child *ptr);
signal_func *set_signal_handler(int signo, signal_func * func);

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
   //struct parser_state parser_state;
   struct __counters counters;

   gettimeofday(&tv1, &tz);

   bzero(&counters, sizeof(counters));

   init_session_data(sdata, &cfg);

   sdata->tot_len = size;

   snprintf(sdata->filename, SMALLBUFSIZE-1, "%s", filename);

   //parser_state = parse_message(sdata, 1, &data, &cfg);
   //post_parse(sdata, &parser_state, &cfg);
   //fix_m_file(sdata->tmpframe, &parser_state);

   int rc = ERR;
   //int rc = perform_checks(sdata, &data, &parser_state, &cfg);
   //unlink(sdata->tmpframe);

   if(rc != ERR) unlink(sdata->filename);

   //update_counters(sdata, &counters);

   gettimeofday(&tv2, &tz);

   /*syslog(LOG_PRIORITY, "%s: %s, size=%d/%d, attachments=%d, reference=%s, "
                        "message-id=%s, retention=%d, delay=%.2f, delays=%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f, status=%s",
                             sdata->filename, sdata->ttmpfile, sdata->tot_len, sdata->stored_len,
                             parser_state.n_attachments, parser_state.reference, parser_state.message_id,
                             parser_state.retention, tvdiff(tv2,tv1)/1000000.0,
                             sdata->__parse/1000000.0,
                             sdata->__post_parse/1000000.0,
                             sdata->__rules/1000000.0,
                             sdata->__header_checks/1000000.0,
                             sdata->__process_message/1000000.0,
                             sdata->__counters/1000000.0,
                             sdata->__pq/1000000.0,
                             status);*/

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
   //struct session_data sdata;
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
