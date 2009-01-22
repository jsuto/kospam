/*
 * clapf.c, 2009.01.21, SJ
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
#include <locale.h>
#include <errno.h>
#include "misc.h"
#include "list.h"
#include "sig.h"
#include "errmsg.h"
#include "session.h"
#include "config.h"

extern char *optarg;
extern int optind;

int sd;
int nconn = 0;
char *configfile = CONFIG_FILE;
struct __config cfg;
struct url *blackhole = NULL;


void clean_exit();
void fatal(char *s);

#ifdef HAVE_LIBCLAMAV
   struct cl_stat dbstat;
   struct cl_limits limits;
   struct cl_engine *engine = NULL;
   const char *dbdir;
   unsigned int options=0;

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
         clean_exit();
      }

      if((retval = cl_build(engine)) != 0){
         syslog(LOG_PRIORITY, "Database initialization error: can't build engine: %s", cl_strerror(retval));
         cl_free(engine);
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

#ifdef HAVE_LIBCLAMAV
   if(engine)
      cl_free(engine);
#endif

   free_list(blackhole);

   syslog(LOG_PRIORITY, "%s has been terminated", PROGNAME);

   unlink(cfg.pidfile);

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
   char *p, puf[SMALLBUFSIZE];

   cfg = read_config(configfile);

   if(chdir(cfg.workdir))
      fatal(ERR_CHDIR);

#ifdef HAVE_LIBCLAMAV

    /* set up archive limits */

    memset(&limits, 0, sizeof(struct cl_limits));

    limits.maxfiles = cfg.clamav_maxfile;                   /* maximum number of files to be scanned
                                                               within a single archive */
    limits.maxfilesize = cfg.clamav_max_archived_file_size; /* compressed files will only be decompressed
                                                               and scanned up to this size */
    limits.maxreclevel = cfg.clamav_max_recursion_level;    /* maximum recursion level for archives */
    limits.archivememlim = cfg.clamav_archive_mem_limit;    /* limit memory usage for some unpackers */


    if(cfg.clamav_use_phishing_db == 1)
       options = CL_DB_STDOPT|CL_DB_PHISHING|CL_DB_PHISHING_URLS;
    else
       options = 0;

#endif

   setlocale(LC_ALL, cfg.locale);

   /* (re)create blackhole list */

   free_list(blackhole);
   blackhole = NULL;

   p = cfg.blackhole_email_list;
   do {
      p = split(p, ' ', puf, SMALLBUFSIZE-1);
      if(strlen(puf) > 3) append_list(&blackhole, puf);
   } while(p);


   syslog(LOG_PRIORITY, "reloaded config: %s", configfile);
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
    FILE *f;

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

    /* go to the background */
    if(daemonise == 1) daemon(1, 0);

    /* write pid file */

    f = fopen(cfg.pidfile, "w");
    if(f){
       fprintf(f, "%d", getpid());
       fclose(f);
    }
    else syslog(LOG_PRIORITY, "cannot write pidfile: %s", cfg.pidfile);

    /* main accept loop */

    for(;;){

       /* let new connections wait if we are too busy now */

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

           /* handle session */

           #ifdef HAVE_LIBCLAMAV
              postfix_to_clapf(new_sd, blackhole, limits, engine, &cfg);
           #else
              postfix_to_clapf(new_sd, blackhole, &cfg);
           #endif

       }

       close(new_sd);
    }

    return 0;
}
