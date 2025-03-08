/*
 * piler.c, SJ
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
#include <locale.h>
#include <errno.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <clapf.h>


extern char *optarg;
extern int optind;

int sd;
int quit = 0;
int received_sighup = 0;
char *configfile = CONFIG_FILE;
struct __config cfg;
struct __data data;
struct passwd *pwd;

struct child children[MAXCHILDREN];


static void takesig(int sig);
static void child_sighup_handler(int sig);
static void child_main(struct child *ptr);
static pid_t child_make(struct child *ptr);
int search_slot_by_pid(pid_t pid);
void kill_children(int sig);
void p_clean_exit();
void fatal(char *s);
void initialise_configuration();




static void takesig(int sig){
   int i, status;
   pid_t pid;
   struct session_data sdata;

   switch(sig){

        case SIGHUP:
                initialise_configuration();
                kill_children(SIGHUP);
                break;

        case SIGTERM:
        case SIGKILL:
                quit = 1;
                p_clean_exit();
                break;

        case SIGCHLD:
                while((pid = waitpid (-1, &status, WNOHANG)) > 0){

                   if(open_database(&sdata, &cfg) == OK){
                      remove_child_stat_entry(&sdata, pid);
                      close_database(&sdata);
                   }

                   if(quit == 0){
                      i = search_slot_by_pid(pid);
                      if(i >= 0){
                         children[i].status = READY;
                         children[i].pid = child_make(&children[i]);
                      }
                      else syslog(LOG_PRIORITY, "error: couldn't find slot for pid %d", pid);

                   }
                }
                break;
   }

   return;
}


static void child_sighup_handler(int sig){
   if(sig == SIGHUP){
      received_sighup = 1;
   }
}


static void child_main(struct child *ptr){
   int new_sd, rc=ERR;
   char s[INET6_ADDRSTRLEN];
   struct sockaddr_storage client_addr;
   socklen_t addr_size;
   struct session_data sdata;

   rc = open_database(&sdata, &cfg);

   sdata.pid = getpid();

   if(rc == OK) create_child_stat_entry(&sdata, getpid());

   ptr->messages = 0;

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "child (pid: %d) started main()", getpid());

   while(1){
      if(received_sighup == 1){
         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "child (pid: %d) caught HUP signal", getpid());
         break;
      }

      ptr->status = READY;

      addr_size = sizeof(client_addr);
      new_sd = accept(sd, (struct sockaddr *)&client_addr, &addr_size);

      if(new_sd == -1) continue;

      ptr->status = BUSY;

      inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof(s));

      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "connection from %s", s);


      sig_block(SIGHUP);
      ptr->messages += handle_smtp_session(new_sd, &data, &cfg);
      sig_unblock(SIGHUP);

      close(new_sd);

      if(rc == OK) update_child_stat_entry(&sdata, 'I', ptr->messages);

      if(cfg.max_requests_per_child > 0 && ptr->messages >= cfg.max_requests_per_child){
         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "child (pid: %d) served enough: %d", getpid(), ptr->messages);
         break;
      }

   }

   ptr->status = UNDEF;

#ifdef HAVE_MEMCACHED
   memcached_shutdown(&(data.memc));
#endif

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "child decides to exit (pid: %d)", getpid());

   if(rc == OK) close_database(&sdata);

   exit(0);
}


static pid_t child_make(struct child *ptr){
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
   }

   for(i=0; i<cfg.number_of_worker_processes; i++){
      children[i].status = READY;
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


void kill_children(int sig){
   int i;

   for(i=0; i<MAXCHILDREN; i++){
      if(children[i].status != UNDEF && children[i].pid > 1){
         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "sending signal to child (pid: %d)", children[i].pid);
         kill(children[i].pid, sig);
      }
   }

}


void p_clean_exit(){
   if(sd != -1) close(sd);

   kill_children(SIGTERM);

   clearhash(data.mydomains);

#ifdef HAVE_TRE
   zombie_free(&data);
#endif

   syslog(LOG_PRIORITY, "%s has been terminated", PROGNAME);

   unlink(cfg.pidfile);

   if(data.ctx){
      SSL_CTX_free(data.ctx);
      ERR_free_strings();
   }

   exit(1);
}


void fatal(char *s){
   syslog(LOG_PRIORITY, "%s\n", s);
   p_clean_exit();
}


int init_ssl(){

   SSL_library_init();
   SSL_load_error_strings();

   data.ctx = SSL_CTX_new(TLS_server_method());

   if(data.ctx == NULL){ syslog(LOG_PRIORITY, "error: SSL_CTX_new() failed"); return ERR; }

   if(SSL_CTX_set_cipher_list(data.ctx, cfg.cipher_list) == 0){ syslog(LOG_PRIORITY, "error: failed to set cipher list: '%s'", cfg.cipher_list); return ERR; }

   if(SSL_CTX_use_PrivateKey_file(data.ctx, cfg.pemfile, SSL_FILETYPE_PEM) != 1){ syslog(LOG_PRIORITY, "error: cannot load private key from %s", cfg.pemfile); return ERR; }

   if(SSL_CTX_use_certificate_file(data.ctx, cfg.pemfile, SSL_FILETYPE_PEM) != 1){ syslog(LOG_PRIORITY, "error: cannot load certificate from %s", cfg.pemfile); return ERR; }

   return OK;
}


void initialise_configuration(){
   struct session_data sdata;

   cfg = read_config(configfile);

   if(cfg.number_of_worker_processes < 5) cfg.number_of_worker_processes = 5;
   if(cfg.number_of_worker_processes > MAXCHILDREN) cfg.number_of_worker_processes = MAXCHILDREN;

   if(strlen(cfg.username) > 1){
      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "username='%s'", cfg.username);

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

   if(cfg.tls_enable > 0 && data.ctx == NULL && init_ssl() == OK){
      snprintf(data.starttls, sizeof(data.starttls)-1, "250-STARTTLS\r\n");
   }

   if(open_database(&sdata, &cfg) == ERR){
      syslog(LOG_PRIORITY, "error: cannot connect to mysql server");
      return;
   }

   init_child_stat_entry(&sdata);

   close_database(&sdata);

#ifdef HAVE_TRE
   zombie_init(&data, &cfg);
#endif

   syslog(LOG_PRIORITY, "reloaded config: %s", configfile);

#ifdef HAVE_MEMCACHED
   memcached_init(&(data.memc), cfg.memcached_servers, 11211);
#endif
}


int main(int argc, char **argv){
   int i, rc, yes=1, daemonise=0;
   char port_string[8];
   struct addrinfo hints, *res;


   while((i = getopt(argc, argv, "c:dvVh")) > 0){
      switch(i){

        case 'c' :
                   configfile = optarg;
                   break;

        case 'd' :
                   daemonise = 1;
                   break;

        case 'v' :
                   printf("%s %s, build %d\n", PROGNAME, VERSION, get_build());
                   break;

        case 'V' :
                   printf("%s %s, build %d, Janos SUTO <sj@acts.hu>\n\n%s\nMySQL client library version: %s\n", PROGNAME, VERSION, get_build(), CONFIGURE_PARAMS, mysql_get_client_info());
                   return 0;

        case 'h' :
        default  :
                   __fatal("\nusage:\n      -c <config file>\n      -d: daemonize\n      -v|-V: show version\n      -h: show this help\n\n");
      }
   }

   (void) openlog(PROGNAME, LOG_PID, LOG_MAIL);

   inithash(data.mydomains);
   data.ctx = NULL;
   data.ssl = NULL;
   data.n_regex = 0;
   memset(data.starttls, 0, sizeof(data.starttls));


   initialise_configuration();

   set_signal_handler (SIGPIPE, SIG_IGN);


   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   snprintf(port_string, sizeof(port_string)-1, "%d", cfg.listen_port);

   if((rc = getaddrinfo(cfg.listen_addr, port_string, &hints, &res)) != 0){
      fprintf(stderr, "getaddrinfo for '%s': %s\n", cfg.listen_addr, gai_strerror(rc));
      return 1;
   }


   if((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
      fatal(ERR_OPEN_SOCKET);

   if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      fatal(ERR_SET_SOCK_OPT);

   syslog(LOG_PRIORITY, "trying to bind to %s:%d", cfg.listen_addr, cfg.listen_port);

   if(bind(sd, res->ai_addr, res->ai_addrlen) == -1)
      fatal(ERR_BIND_TO_PORT);

   if(listen(sd, cfg.backlog) == -1)
      fatal(ERR_LISTEN);


   freeaddrinfo(res);


   if(drop_privileges(pwd)) fatal(ERR_SETUID);

   check_and_create_directories(&cfg);

   if(cfg.history == 0) create_partition(&cfg);

   syslog(LOG_PRIORITY, "%s %s, build %d starting", PROGNAME, VERSION, get_build());


#if HAVE_DAEMON == 1
   if(daemonise == 1) i = daemon(1, 0);
#endif

   write_pid_file(cfg.pidfile);


   child_pool_create();

   set_signal_handler(SIGCHLD, takesig);
   set_signal_handler(SIGTERM, takesig);
   set_signal_handler(SIGKILL, takesig);
   set_signal_handler(SIGHUP, takesig);

   for(;;){ sleep(1); }

   p_clean_exit();

   return 0;
}
