#include <kospam.h>

struct child children[MAXCHILDREN];
int quit = 0;
int received_sighup = 0;

struct config cfg;
struct data data;



void child_sighup_handler(int sig){
   if(sig == SIGHUP){
      received_sighup = 1;
   }
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

int child_pool_create(int number_of_worker_processes){
   int i;

   for(i=0; i<MAXCHILDREN; i++){
      children[i].pid = 0;
      children[i].messages = 0;
      children[i].status = UNDEF;
      children[i].serial = -1;
   }

   for(i=0; i<number_of_worker_processes; i++){
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

#ifdef HAVE_TRE
   zombie_free(&data);
#endif

   syslog(LOG_PRIORITY, "%s has been terminated", PROGNAME);

   unlink(cfg.pidfile);

   closelog();

   exit(0);
}

void takesig(int sig){
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
