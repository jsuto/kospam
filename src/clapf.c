/*
 * clapf.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
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
#include "misc.h"
#include "list.h"
#include "sig.h"
#include "errmsg.h"
#include "session.h"
#include "config.h"
#include "clapf.h"

extern char *optarg;
extern int optind;

int sd;
int nconn = 0;
char *configfile = CONFIG_FILE;
struct __config cfg;
struct __data data;
struct passwd *pwd;

#ifdef HAVE_LIBCLAMAV
   unsigned int options=0;
#endif

void fatal(char *s);
void clean_exit();
void fatal(char *s);
void initialiseConfiguration();
void dropPrivileges();
void writePidFile(char *pidfile);
void sigchld();
void reloadClamavDB();



int main(int argc, char **argv){
    int i, new_sd, yes=1, pid, daemonise=0;
    unsigned int clen;
    struct sockaddr_in client_addr, serv_addr;
    struct in_addr addr;

    while((i = getopt(argc, argv, "c:dvVh")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'd' :
                    daemonise = 1;
                    break;

         case 'v' :
         case 'V' :
                    __fatal(PROGNAME " " PROGINFO);
                    break;

         case 'h' :
         default  : 
                    __fatal(CLAPFUSAGE);
       }
    }

    (void) openlog(PROGNAME, LOG_PID, LOG_MAIL);

    sig_catch(SIGINT, clean_exit);
    sig_catch(SIGQUIT, clean_exit);
    sig_catch(SIGKILL, clean_exit);
    sig_catch(SIGTERM, clean_exit);
    sig_catch(SIGHUP, initialiseConfiguration);

    data.blackhole = NULL;

    #ifdef HAVE_TRE
       data.n_regex = 0;
    #endif
    #ifdef HAVE_LIBCLAMAV
       data.engine = NULL;
       sig_catch(SIGALRM, reloadClamavDB);
    #endif


    sig_block(SIGCHLD);
    sig_catch(SIGCHLD, sigchld);


    initialiseConfiguration();


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


    dropPrivileges();


    syslog(LOG_PRIORITY, "%s %s starting", PROGNAME, VERSION);


    #ifdef HAVE_LIBCLAMAV
       reloadClamavDB();
    #endif


#if HAVE_DAEMON == 1
    if(daemonise == 1) i = daemon(1, 0);
#endif

    writePidFile(cfg.pidfile);


    /* main accept loop */

    for(;;){

       /* let new connections wait if we are too busy now */

       if(nconn >= cfg.max_connections) sig_pause();

       clen = sizeof(client_addr);

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
           /* child process */

           sig_uncatch(SIGCHLD);
           sig_unblock(SIGCHLD);

           sig_uncatch(SIGINT);
           sig_uncatch(SIGQUIT);
           sig_uncatch(SIGKILL);
           sig_uncatch(SIGTERM);
           sig_block(SIGHUP);

           handleSession(new_sd, &data, &cfg);
       }

       close(new_sd);
    }

    return 0;
}


void fatal(char *s){
   syslog(LOG_PRIORITY, "%s\n", s);
   clean_exit();
}


void clean_exit(){
   if(sd != -1) close(sd);

#ifdef HAVE_LIBCLAMAV
   if(data.engine) cl_engine_free(data.engine);
#endif

   freeList(data.blackhole);

#ifdef HAVE_TRE
   freeZombieList(&data);
#endif

   syslog(LOG_PRIORITY, "%s has been terminated", PROGNAME);

   unlink(cfg.pidfile);

   exit(1);
}


void initialiseConfiguration(){
   char *p, puf[SMALLBUFSIZE];

   cfg = read_config(configfile);

   if(strlen(cfg.username) > 1){
      pwd = getpwnam(cfg.username);
   }


   if(getuid() == 0 && pwd){
      checkAndCreateClapfDirectories(&cfg, pwd->pw_uid, pwd->pw_gid);
   }


   if(chdir(cfg.workdir)){
      syslog(LOG_PRIORITY, "workdir: *%s*", cfg.workdir);
      fatal(ERR_CHDIR);
   }

#ifdef HAVE_LIBCLAMAV
   cl_engine_set_num(data.engine, CL_ENGINE_MAX_FILES, cfg.clamav_maxfile);
   cl_engine_set_num(data.engine, CL_ENGINE_MAX_FILESIZE, cfg.clamav_max_archived_file_size);
   cl_engine_set_num(data.engine, CL_ENGINE_MAX_RECURSION, cfg.clamav_max_recursion_level);

   options = 0;

   if(cfg.clamav_use_phishing_db == 1) options = CL_DB_STDOPT|CL_DB_PHISHING|CL_DB_PHISHING_URLS;
#endif

   setlocale(LC_MESSAGES, cfg.locale);
   setlocale(LC_CTYPE, cfg.locale);

   freeList(data.blackhole);
   data.blackhole = NULL;

   p = cfg.blackhole_email_list;
   do {
      p = split(p, ' ', puf, SMALLBUFSIZE-1);
      if(strlen(puf) > 3) append_list(&(data.blackhole), puf);
   } while(p);


   syslog(LOG_PRIORITY, "reloaded config: %s", configfile);

#ifdef HAVE_TRE
   initialiseZombieList(&data, &cfg);
#endif


#ifdef HAVE_MEMCACHED
   memcached_init(&(data.memc), cfg.memcached_servers, 11211);
#endif
}


void dropPrivileges(){

   if(pwd->pw_uid > 0 && pwd->pw_gid > 0){

      if(getgid() != pwd->pw_gid){
         if(setgid(pwd->pw_gid)) fatal(ERR_SETGID);
      }

      if(getuid() != pwd->pw_uid){
         if(setuid(pwd->pw_uid)) fatal(ERR_SETUID);
      }

   }
}


void writePidFile(char *pidfile){
   FILE *f;

   f = fopen(pidfile, "w");
   if(f){
      fprintf(f, "%d", (int)getpid());
      fclose(f);
   }
   else syslog(LOG_PRIORITY, "cannot write pidfile: %s", cfg.pidfile);
}


void sigchld(){
   int pid, wstat;

   while((pid = wait_nohang(&wstat)) > 0){
      if(nconn > 0) nconn--;
   }
}


#ifdef HAVE_LIBCLAMAV
void reloadClamavDB(){
   int retval;
   unsigned int sigs = 0;
   const char *dbdir;
   struct cl_stat dbstat;

   if(data.engine){
      cl_engine_free(data.engine);
      cl_statfree(&dbstat);
      data.engine = NULL;
   }

   if((retval = cl_init(CL_INIT_DEFAULT)) != CL_SUCCESS) fatal(ERR_INIT_ERROR);

   dbdir = cl_retdbdir();
   if(dbdir == NULL) fatal(ERR_NO_DB_DIR);

   memset(&dbstat, 0, sizeof(struct cl_stat));
   if(cl_statinidir(dbdir, &dbstat) != 0) fatal(ERR_STAT_INI_DIR);

   if(!(data.engine = cl_engine_new())) fatal("Can't create new engine");


   if(cfg.clamav_use_phishing_db == 1){
      options |= CL_DB_PHISHING;
      options |= CL_DB_PHISHING_URLS;
   }

   if((retval = cl_load(cl_retdbdir(), data.engine, &sigs, options)) != CL_SUCCESS){
      syslog(LOG_PRIORITY, "reloading db failed: %s", cl_strerror(retval));
      clean_exit();
   }

   if((retval = cl_engine_compile(data.engine)) != CL_SUCCESS){
      syslog(LOG_PRIORITY, "Database initialization error: can't build engine: %s", cl_strerror(retval));
      cl_engine_free(data.engine);
      clean_exit();
   }

   syslog(LOG_PRIORITY, "reloaded with %d viruses", sigs);
}
#endif

