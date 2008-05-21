/*
 * ooop.c, 2008.05.20, SJ
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
#include <openssl/ssl.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <clapf.h>
#include "sig.h"
#include "cfg.h"
#include "ooop-util.h"


extern char *optarg;
extern int optind;

int sd;
int nconn = 0;
char *configfile = CONFIG_FILE;
struct __config cfg;


void ooop(int new_sd, int use_ssl, SSL *ssl, struct __config cfg);


/*
 * release everything before we exit
 */

void clean_exit(){
   if(sd != -1)
      close(sd);

   syslog(LOG_PRIORITY, "%s has been terminated", OOOPNAME);
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

   /*if(chdir(cfg.workdir))
      fatal(ERR_CHDIR);*/

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
   int uid=0, gid=0, use_ssl=0;
   unsigned int clen;
   struct sockaddr_in client_addr, server_addr;
   struct in_addr addr;

   /*SSL_CTX *ctx = NULL;
   SSL *ssl = NULL;
   SSL_METHOD *meth = NULL;*/

   while((i = getopt(argc, argv, "c:u:g:dVh")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'u' :
                    uid = atoi(optarg);
                    break;


         case 'g' :
                    gid = atoi(optarg);
                    break;


         case 'd' :
                    daemonise = 1;
                    break;

         case 'V' :
                    __fatal(OOOPNAME " " VERSION);
                    break;

         case 'h' :
         default  : 
                    __fatal("usage: " OOOPNAME " -c <config file> -u <uid> -g <gid> -d");
       }
   }

   (void) openlog(OOOPNAME, LOG_PID, LOG_MAIL);

   signal(SIGINT, clean_exit);
   signal(SIGQUIT, clean_exit);
   signal(SIGKILL, clean_exit);
   signal(SIGTERM, clean_exit);
   signal(SIGHUP, reload_config);

   sig_block(SIGCHLD);
   sig_catch(SIGCHLD, sigchld);

   reload_config();

   if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      fatal(ERR_OPEN_SOCKET);


   /*SSL_load_error_strings();
   SSLeay_add_ssl_algorithms();
   meth = SSLv23_server_method();

   ctx = SSL_CTX_new(meth);
   if(ctx == NULL) fatal("ctx error");

   if(SSL_CTX_use_certificate_file(ctx, cfg.ssl_cert_file, SSL_FILETYPE_PEM) <= 0)
      _fatal("cannot read certificate file");

   if(SSL_CTX_use_PrivateKey_file(ctx, cfg.ssl_key_file, SSL_FILETYPE_PEM) <= 0)
      _fatal("cannot read private key file");

   if(!SSL_CTX_check_private_key(ctx))
      _fatal("Private key does not match the certificate public key");*/


   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(cfg.listen_port);
   inet_aton(cfg.listen_addr, &addr);
   server_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(server_addr.sin_zero), 8);

   if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
       fatal(ERR_SET_SOCK_OPT);

   if(bind(sd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
       fatal(ERR_BIND_TO_PORT);

   if(listen(sd, cfg.backlog) == -1)
       fatal(ERR_LISTEN);



   /* we do not need root privs any longer */
   drop_root(uid, gid);


   syslog(LOG_PRIORITY, "%s %s starting", OOOPNAME, VERSION);


   clen = sizeof(client_addr);

   /* go to the background */
   if(daemonise == 1) daemon(1, 0);


   /* main accept loop */

   for(;;){

      /* let new connections wait if we are too busy now */

      if(nconn >= cfg.max_connections) sig_pause();

      sig_unblock(SIGCHLD);
      new_sd = accept(sd, (struct sockaddr *)&client_addr, &clen);
      sig_block(SIGCHLD);

      if(new_sd == -1) continue;

      if(strcmp(inet_ntoa(client_addr.sin_addr), "127.0.0.1") == 0) use_ssl = 1;
      else use_ssl = 0;

      syslog(LOG_PRIORITY, "connection from client: %s, ssl: %d", inet_ntoa(client_addr.sin_addr), use_ssl);

      nconn++;

      pid = fork();

      if(pid != 0){
         if(pid == -1){
            nconn--;
            syslog(LOG_PRIORITY, "%s", ERR_FORK_FAILED);
         }
      }
      else {

         sig_uncatch(SIGCHLD);
         sig_unblock(SIGCHLD);

         ooop(new_sd, use_ssl, NULL, cfg);
      }

      close(new_sd);

   }

   return 0;
}
