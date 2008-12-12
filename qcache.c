/*
 * qcache.c, 2008.12.12, SJ
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
#include "mydb.h"
#include <clapf.h>

#define QCACHE_PROGNAME "Qcache"

extern char *optarg;
extern int optind;

int sd;
int nconn = 0;
char *configfile = CONFIG_FILE;
struct __config cfg;
struct mydb_node *mhash[MAX_MYDB_HASH], **m;
struct session_data sdata;

float calc_spamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x);


/*
 * release everything before we exit
 */

void clean_exit(){
   if(sd != -1)
      close(sd);

   close_mydb(mhash);
   printf("terminated\n");
   syslog(LOG_PRIORITY, "%s has been terminated", QCACHE_PROGNAME);
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
   syslog(LOG_PRIORITY, "reloaded config: %s", configfile);
}


float xqry(struct mydb_node **xhash, unsigned long long key, struct session_data *sdata, float rob_s, float rob_x){
   struct mydb_node *q;


   q = findmydb_node(xhash, key);
   if(q == NULL) return DEFAULT_SPAMICITY;

   return calc_spamicity(sdata->Nham, sdata->Nspam, q->nham, q->nspam, rob_s, rob_x);
}


int main(int argc, char **argv){
   int i, j, new_sd=-1, yes=1, daemonise=0;
   unsigned int clen;
   struct sockaddr_in client_addr, serv_addr;
   struct in_addr addr;
   fd_set master;
   fd_set read_fds;
   int fdmax;
   struct x_token *x;


   while((i = getopt(argc, argv, "c:u:g:dVh")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'd' :
                    daemonise = 1;
                    break;

         case 'V' :
                    __fatal(QCACHE_PROGNAME " " VERSION);
                    break;

         case 'h' :
         default  : 
                    __fatal("usage: " QCACHE_PROGNAME " -c <config file> -d");
       }
   }

   (void) openlog(QCACHE_PROGNAME, LOG_PID, LOG_MAIL);

   signal(SIGINT, clean_exit);
   signal(SIGQUIT, clean_exit);
   signal(SIGKILL, clean_exit);
   signal(SIGTERM, clean_exit);
   signal(SIGHUP, reload_config);

   reload_config();

   if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      fatal(ERR_OPEN_SOCKET);

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(MY_PORT);
   inet_aton(cfg.listen_addr, &addr);
   serv_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(serv_addr.sin_zero), 8);

   if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
       fatal(ERR_SET_SOCK_OPT);

   if(bind(sd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
       fatal(ERR_BIND_TO_PORT);

   if(listen(sd, cfg.backlog) == -1)
       fatal(ERR_LISTEN);


   syslog(LOG_PRIORITY, "%s %s starting", QCACHE_PROGNAME, VERSION);


   clen = sizeof(client_addr);

   init_mydb(cfg.mydbfile, mhash, &sdata);

   m = &mhash[0];

   /* go to the background */
   if(daemonise == 1) daemon(1, 0);

   FD_ZERO(&master);
   FD_ZERO(&read_fds);

   FD_SET(sd, &master);

   fdmax = sd;

   /* main accept loop */

   int nbytes;
   char buf[PKT_SIZE];

   for(;;){
      read_fds = master; // copy it
      if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
         perror("select");
         exit(1);
      }


      /* run through the existing connections looking for data to read */

      for(i=0; i <= fdmax; i++){
         if(FD_ISSET(i, &read_fds)){ // we got one!!

            /* handle new connections */

            if(i == sd){
               if((new_sd = accept(sd, (struct sockaddr *)&client_addr, &clen)) != -1){
                  FD_SET(new_sd, &master);
                  if(new_sd > fdmax) fdmax = new_sd;

                  //printf("selectserver: new connection from %s on socket %d\n", inet_ntoa(client_addr.sin_addr), new_sd);
               }
            }

            else{
               /* handle data from a client */

               if((nbytes = recv(i, buf, sizeof buf, 0)) <= 0){

                  /* let's close the connection */

                  if(nbytes == 0){
                     //printf("selectserver: socket %d hung up\n", i);
                  }
                  close(i);
                  FD_CLR(i, &master);

                  if(i == new_sd) new_sd = -1;

               } else {
                  /* we got some data from a client */

                  for(j=0; j<NETWORK_SEGMENT_SIZE; j++){
                     x = (struct x_token*)(buf + j*sizeof(struct x_token));
                     if(x->token > 0) x->spaminess = xqry(m, x->token, &sdata, cfg.rob_s, cfg.rob_x);
                  }

                  send(new_sd, buf, PKT_SIZE, 0);
               }

            }


         }
      }

   }

   return 0;
}
