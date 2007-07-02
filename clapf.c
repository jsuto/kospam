/*
 * clapf.c, 2007.06.01, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "misc.h"
#include "sig.h"
#include "errmsg.h"
#include "session.h"
#include "hash_db.h"
#include "config.h"

extern char *optarg;
extern int optind;

int sd;
int nconn = 0;
char *configfile = CONFIG_FILE;
struct __config cfg;

#ifdef HAVE_HASH_DB
   int t_hash_ready = 0;
#endif

void clean_exit();
void fatal(char *s);

#ifdef HAVE_LIBCLAMAV
   struct cl_node *root = NULL;
   struct cl_stat dbstat;
   struct cl_limits limits;
   struct cl_engine *engine = NULL;
   const char *dbdir;

   void reload_clamav_db(){
      int retval;
      unsigned int sigs = 0;

      /* release old structure */
      if(root){
         cl_free(root);
         root = NULL;
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
      if((retval = cl_load(cl_retdbdir(), &engine, &sigs, CL_DB_STDOPT|CL_DB_PHISHING|CL_DB_PHISHING_URLS))){
         syslog(LOG_PRIORITY, "reloading db failed: %s", cl_strerror(retval));
         clean_exit();
      }

      if(!root){
         syslog(LOG_PRIORITY, "loading db failed");
         clean_exit();
      }

      if((retval = cl_build(root)) != 0){
         syslog(LOG_PRIORITY, "Database initialization error: can't build engine: %s", cl_strerror(retval));
         clean_exit();
      }

      syslog(LOG_PRIORITY, "reloaded with %d viruses", sigs);
   }

#endif


/*
 * release everything before we exit
 */

void clean_exit(){
   if(sd != -1)
      close(sd);

#ifdef HAVE_HASH_DB
   clear_token_hash(t_hash);
#endif

#ifdef HAVE_LIBCLAMAV
   if(root)
      cl_free(root);
#endif

   syslog(LOG_PRIORITY, "%s has been terminated", PROGNAME);
   exit(1);
}

/*
 * exit with an error string
 */

void fatal(char *s){
   syslog(LOG_PRIORITY, "%s\n", s);
   clean_exit();
}

/*
 * reload configuration
 */

void reload_config(){
   cfg = read_config(configfile);

   if(chdir(cfg.workdir))
      fatal(ERR_CHDIR);

   /* reload the memory storage */

   #ifdef HAVE_HASH_DB
      if(t_hash_ready == 1){
         clear_token_hash(t_hash);
         if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "freed the token database: %s", cfg.raw_text_datafile);
      }

      init_token_hash(t_hash);

      t_hash_ready = read_datafile(cfg.raw_text_datafile, t_hash);

      if(t_hash_ready == 1)
         syslog(LOG_PRIORITY, "read the raw token file successfully: %s", cfg.raw_text_datafile);
      else
         syslog(LOG_PRIORITY, "failed to read raw tokens file: %s", cfg.raw_text_datafile);
   #endif

   #ifdef HAVE_LIBCLAMAV

       /* set up archive limits */

       memset(&limits, 0, sizeof(struct cl_limits));

       limits.maxfiles = cfg.clamav_maxfile;
       limits.maxfilesize = cfg.clamav_max_archived_file_size;
       limits.maxreclevel = cfg.clamav_max_recursion_level;
       limits.maxratio = cfg.clamav_max_compress_ratio;
       limits.archivememlim = cfg.clamav_archive_mem_limit;

   #endif

   if(configfile)
      syslog(LOG_PRIORITY, "reloaded config: %s", configfile);
   else
      syslog(LOG_PRIORITY, "reloaded default config");
}

/*
 * child handler
 */

void sigchld(){
  int wstat;
  int pid;

  while((pid = wait_nohang(&wstat)) > 0){
    if(nconn > 0) nconn--;
  }
}

int main(int argc, char **argv){
    int i, new_sd, yes=1, pid, daemonise=0;
    unsigned int clen;
    struct sockaddr_in client_addr, serv_addr;
    struct in_addr addr;

    while((i = getopt(argc, argv, "c:dVhQ")) > 0){
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
       }
    }

    (void) openlog(PROGNAME, LOG_PID, LOG_MAIL);

    signal(SIGINT, clean_exit);
    signal(SIGQUIT, clean_exit);
    signal(SIGKILL, clean_exit);
    signal(SIGTERM, clean_exit);
    signal(SIGHUP, reload_config);

    #ifdef HAVE_LIBCLAMAV
       signal(SIGALRM, reload_clamav_db);
    #endif

    sig_block(SIGCHLD);
    sig_catch(SIGCHLD, sigchld);

    reload_config();

    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
       fatal(ERR_OPEN_SOCKET);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(cfg.listen_port);
    inet_aton(cfg.listen_addr, &addr);
    serv_addr.sin_addr.s_addr = addr.s_addr;
    bzero(&(serv_addr.sin_zero), 8);

    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
       fatal(ERR_SET_SOCK_OPT);

    if(bind(sd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
        fatal(ERR_BIND_TO_PORT);

    if(listen(sd, cfg.backlog) == -1)
        fatal(ERR_LISTEN);

    syslog(LOG_PRIORITY, "%s %s starting", PROGNAME, VERSION);

    /* libclamav startup */

    #ifdef HAVE_LIBCLAMAV
       reload_clamav_db();
    #endif


    clen = sizeof(client_addr);

    /* go to the background, 2007.06.01, SJ */
    if(daemonise == 1) daemon(1, 0);


    /* main accept loop */

    for(;;){

       /* let new connections wait if we are too busy now, 2006.01.13, SJ */

       if(nconn >= cfg.max_connections) sig_pause();

       sig_unblock(SIGCHLD);
       new_sd = accept(sd, (struct sockaddr *)&client_addr, &clen);
       sig_block(SIGCHLD);

       if(new_sd == -1)
           continue;

       nconn++;

       pid = fork();

       if(pid != 0){
          if(pid == -1){
             nconn--;
             syslog(LOG_PRIORITY, "%s", ERR_FORK_FAILED);
          }
       }
       else {
           /* child process, do initialisation */

           sig_uncatch(SIGCHLD);
           sig_unblock(SIGCHLD);

           /* handle session, 2006.02.11, SJ */
           #ifdef HAVE_LIBCLAMAV
              postfix_to_clapf(new_sd, cfg, limits, root);
           #else
              postfix_to_clapf(new_sd, cfg);
           #endif

       }

       close(new_sd);
    }

    return 0;
}
