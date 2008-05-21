/*
 * remote_auth.c, 2008.05.18, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include "misc.h"
#include "pop3_messages.h"
#include "ooop-util.h"
#include "config.h"


int remote_auth(char *username, char *password, int *sd, SSL **ssl, int use_ssl, char *msg){
   int n, ret=0, port=POP3_LISTEN_PORT;
   char *p, buf[MAXBUFSIZE];
   struct sockaddr_in remote_addr;

   memset(msg, 0, SMALLBUFSIZE);

   if(!username || !password){
      strncpy(msg, AUTH_ERR_MISSING_USERNAME_OR_PASSWORD, SMALLBUFSIZE-1);
      return ret;
   }

   /* so we got username in "realusername:pop3_server" format */

   p = strchr(username, ':');
   if(p){
      *p = '\0';
      p++;
   }
   else {
      strncpy(msg, AUTH_ERR_NO_COLON_IN_USERNAME, SMALLBUFSIZE-1);
      return ret;
   }

   if(use_ssl == 1){
      port = POP3_LISTEN_SSL_PORT;
      if(SSL_set_fd(*ssl, *sd) != 1) goto ENDE;
   }


   remote_addr.sin_family = AF_INET;
   remote_addr.sin_port = htons(port);
   remote_addr.sin_addr.s_addr = resolve_host(p);
   if(remote_addr.sin_addr.s_addr == 0){
      strncpy(msg, AUTH_ERR_INVALID_HOSTNAME, SMALLBUFSIZE-1);
      goto ENDE;
   }
   bzero(&(remote_addr.sin_zero), 8);

   /* let's connect to remote server and try this account info */

   if(connect(*sd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1){
      syslog(LOG_PRIORITY, "ERR: connect to %s:%d failed", p, port);
      strncpy(msg, AUTH_ERR_CANNOT_CONNECT, SMALLBUFSIZE-1);
      goto ENDE;
   }

   if(use_ssl == 1){
      if(SSL_connect(*ssl) == -1) goto ENDE;      
   }


   /* read POP3 banner */

   n = recvtimeoutssl(*sd, buf, MAXBUFSIZE, TIMEOUT, use_ssl, *ssl);


   /* send USER */

   snprintf(buf, MAXBUFSIZE-1, "USER %s\r\n", username);
   write1(*sd, buf, use_ssl, *ssl);

   n = recvtimeoutssl(*sd, buf, MAXBUFSIZE, TIMEOUT, use_ssl, *ssl);


   /* send PASS */

   snprintf(buf, MAXBUFSIZE-1, "PASS %s\r\n", password);
   write1(*sd, buf, use_ssl, *ssl);

   n = recvtimeoutssl(*sd, buf, MAXBUFSIZE, TIMEOUT, use_ssl, *ssl);

   snprintf(msg, SMALLBUFSIZE-1, "%s", buf);

   if(strncmp(buf, "+OK", 3) == 0) ret = 1;

ENDE:

   return ret;
}

