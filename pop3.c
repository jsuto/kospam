/*
 * smap.c, 2007.08.20, SJ
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
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "errmsg.h"
#include "misc.h"
#include "sig.h"
#include "cfg.h"
#include "ssl_util.h"
#include "config.h"


extern char *optarg;
extern int optind;

int sd;
int nconn = 0;
char *configfile = CONFIG_FILE;
struct __config cfg;


void pop3_session(int new_sd, struct __config cfg);

/*
 * release everything before we exit
 */

void clean_exit(){
   if(sd != -1)
      close(sd);

   LOG_MESSAGE(POP3_PROGNAME " terminated");
   exit(1);
}

/*
 * exit with an error string
 */

void fatal(char *s){
   LOG_MESSAGE(s);
   clean_exit();
}

/*
 * reload configuration
 */

void reload_config(){
   char buf[SMALLBUFSIZE];

   cfg = read_config(configfile);

   snprintf(buf, SMALLBUFSIZE-1, "entering to working directory: %s", cfg.workdir);
   LOG_MESSAGE(buf);

   if(chdir(cfg.workdir))
      fatal(ERR_CHDIR);

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
   int new_sd, yes=1, pid, daemonise=0;
   unsigned int clen;
   struct sockaddr_in client_addr, serv_addr;
   struct in_addr addr;


   (void) openlog(POP3_PROGNAME, LOG_PID, LOG_MAIL);

   signal(SIGINT, clean_exit);
   signal(SIGQUIT, clean_exit);
   signal(SIGKILL, clean_exit);
   signal(SIGTERM, clean_exit);
   signal(SIGHUP, reload_config);

   sig_block(SIGCHLD);
   sig_catch(SIGCHLD, sigchld);

   if(argc > 1) configfile = argv[1];

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

   LOG_MESSAGE(POP3_PROGNAME " " VERSION " is starting");


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
             LOG_MESSAGE(ERR_FORK_FAILED);
          }
       }
       else {
          /* child process, do initialisation */

          sig_uncatch(SIGCHLD);
          sig_unblock(SIGCHLD);

          /* handle pop3 session */
          pop3_session(new_sd, cfg);

      }

      close(new_sd);
   }

   return 0;
}
