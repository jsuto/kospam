/*
 * clapf.c, 2008.02.07, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "misc.h"
#include "errmsg.h"
#include "smtpcodes.h"
#include "session.h"
#include "config.h"


extern char *optarg;
extern int optind;


char *configfile = CONFIG_FILE;
struct __config cfg;


typedef struct {
   int sockfd;
   pthread_t thread;
} qconn;


#ifdef HAVE_LIBCLAMAV
   struct cl_stat dbstat;
   struct cl_limits limits;
   struct cl_engine *engine = NULL;
   const char *dbdir;
   unsigned int options=0;
#endif

struct timezone tz;
pthread_mutex_t __lock;
pthread_attr_t attr;
int __num_threads, listener;
char too_many_connections[SMALLBUFSIZE];

/*
 * shutdown clapf
 */

void clapf_exit(){
   close(listener);
   pthread_attr_destroy(&attr);

#ifdef HAVE_LIBCLAMAV
   if(engine)
      cl_free(engine);
#endif

   syslog(LOG_PRIORITY, "%s has been terminated", PROGNAME);

   unlink(cfg.pidfile);

   exit(1);
}


/*
 * exit with a message
 */

void fatal(char *s){
   printf("%s\n", s);
   clapf_exit();
}


/*
 * increment thread counter
 */

void increment_thread_count(void){
   pthread_mutex_lock(&__lock);
   __num_threads++;
   pthread_mutex_unlock(&__lock);
}


/*
 * decrement thread counter
 */

void decrement_thread_count(void){
   pthread_mutex_lock(&__lock);
   __num_threads--;
   pthread_mutex_unlock(&__lock);
}


/*
 * reload configuration
 */

void reload_config(){
   cfg = read_config(configfile);

   snprintf(too_many_connections, SMALLBUFSIZE-1, SMTP_RESP_421_ERR_TMP, cfg.hostid);

   if(chdir(cfg.workdir))
      fatal(ERR_CHDIR);

#ifdef HAVE_LIBCLAMAV
    /* set up archive limits */

    memset(&limits, 0, sizeof(struct cl_limits));

    limits.maxfiles = cfg.clamav_maxfile;
    limits.maxfilesize = cfg.clamav_max_archived_file_size;
    limits.maxreclevel = cfg.clamav_max_recursion_level;
    limits.maxratio = cfg.clamav_max_compress_ratio;
    limits.archivememlim = cfg.clamav_archive_mem_limit;

    if(cfg.clamav_use_phishing_db == 1)
       options = CL_DB_STDOPT|CL_DB_PHISHING|CL_DB_PHISHING_URLS;
    else
       options = 0;
#endif

   syslog(LOG_PRIORITY, "reloaded config: %s", configfile);
}


#ifdef HAVE_LIBCLAMAV
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
      clapf_exit();
   }

   if((retval = cl_build(engine)) != 0){
      syslog(LOG_PRIORITY, "Database initialization error: can't build engine: %s", cl_strerror(retval));
      cl_free(engine);
      clapf_exit();
   }

   syslog(LOG_PRIORITY, "reloaded with %d viruses", sigs);
}
#endif



/*
 * process a connection
 */

void *process_connection(void *ptr){
   qconn *QC = (qconn*)ptr;

#ifdef HAVE_LIBCLAMAV
   postfix_to_clapf(QC->sockfd, cfg, limits, engine);
#else
   postfix_to_clapf(QC->sockfd, cfg);
#endif

   close(QC->sockfd);

   decrement_thread_count();
   pthread_exit(0);

   return 0;
}


int main(int argc, char **argv){
   int i, fdmax, newfd, addrlen, yes=1, daemonise=0;
   struct sockaddr_in remote_addr;
   struct sockaddr_in local_addr;
   struct in_addr addr;
   struct timeval tv;
   qconn *QC;
   fd_set master, read_fds;
   FILE *f;


   while((i = getopt(argc, argv, "c:dVh")) > 0){
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
                    break;

       }
   }


   /* initialize */


   (void) openlog(PROGNAME, LOG_PID, LOG_MAIL);

   signal(SIGINT, clapf_exit);
   signal(SIGQUIT, clapf_exit);
   signal(SIGKILL, clapf_exit);
   signal(SIGTERM, clapf_exit);
   signal(SIGHUP, reload_config);

#ifdef HAVE_LIBCLAMAV
   signal(SIGALRM, reload_clamav_db);
#endif

   reload_config();

   /* libclamav startup */

#ifdef HAVE_LIBCLAMAV
   reload_clamav_db();
#endif

   FD_ZERO(&master);
   FD_ZERO(&read_fds);

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   __num_threads=0;


   /* create a listener socket */

   if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      fatal(ERR_OPEN_SOCKET);

   local_addr.sin_family = AF_INET;
   local_addr.sin_port = htons(cfg.listen_port);
   inet_aton(cfg.listen_addr, &addr);
   local_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(local_addr.sin_zero), 8);

   if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      fatal(ERR_SET_SOCK_OPT);

   if(bind(listener, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1)
      fatal(ERR_BIND_TO_PORT);

   if(listen(listener, cfg.backlog) == -1)
      fatal(ERR_LISTEN);

   FD_SET(listener, &master);
   fdmax = listener;

   syslog(LOG_PRIORITY, "%s %s starting", PROGNAME, VERSION);

   /* write pid file */

   f = fopen(cfg.pidfile, "w");
   if(f){
      fprintf(f, "%d", getpid());
      fclose(f);
   }
   else syslog(LOG_PRIORITY, "cannot write pidfile: %s", cfg.pidfile);

   if(daemonise == 1) daemon(1, 0);


   /* main loop */
 
   for(;;) {
      read_fds = master;

      tv.tv_sec = 2;
      tv.tv_usec = 0;


      if(select(fdmax+1, &read_fds, NULL, NULL, &tv) > 0){

         for(i=0;i<=fdmax;i++) {
            if(FD_ISSET(i, &read_fds)){

               /* handle connections */

               if(i == listener){
                  addrlen = sizeof(remote_addr);

                  if((newfd = accept(listener, (struct sockaddr *)&remote_addr, (socklen_t *) &addrlen)) == -1){
                     fprintf(stderr, "daemon error accept\n");
                     continue;
                  }
                  fcntl(newfd, F_SETFL, O_RDWR);
                  setsockopt(newfd, SOL_SOCKET, TCP_NODELAY, &yes, sizeof(int));

                  if(__num_threads >= cfg.max_connections){
                     send(newfd, too_many_connections, strlen(too_many_connections), 0);
                     goto DEFERRED;
                  }

                  QC = malloc(sizeof(qconn));
                  if(QC == NULL) goto DEFERRED;

                  increment_thread_count();
                  QC->sockfd = newfd;


                  if(pthread_create(&QC->thread, &attr, process_connection, (void *)QC)){
                     decrement_thread_count();

                  DEFERRED:
                     close(newfd);
                     continue;
                  }

               }
            }
         }
      }
   }



   return 0;
}
