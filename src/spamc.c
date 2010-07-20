/*
 * spamc.c, SJ
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <clapf.h>


#define SPAMD_RESP_SPAM_STRING "Spam: True ; "
#define SPAMD_RESP_SPAM_STRING_LENGTH strlen(SPAMD_RESP_SPAM_STRING)


int spamc_emul(char *tmpfile, int size, struct __config *cfg){
   int fd, n, psd, rc=0;
   char *p, buf[MAXBUFSIZE];
   struct in_addr addr;
   struct sockaddr_in avast_addr;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to pass to avast!", tmpfile);

   if((psd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      syslog(LOG_PRIORITY, "%s: ERR: create socket", tmpfile);
      return rc;
   }

   avast_addr.sin_family = AF_INET;
   avast_addr.sin_port = htons(cfg->spamd_port);
   inet_aton(cfg->spamd_addr, &addr);
   avast_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(avast_addr.sin_zero), 8);

   if(connect(psd, (struct sockaddr *)&avast_addr, sizeof(struct sockaddr)) == -1){
      syslog(LOG_PRIORITY, "%s: cannot create socket", tmpfile);
      return 0;
   }

   snprintf(buf, MAXBUFSIZE-1, "PROCESS SPAMC/1.3\r\nUser: %s\r\nContent-length: %d\r\n\r\n", cfg->spamc_user, size);
   send(psd, buf, strlen(buf), 0);

   fd = open(tmpfile, O_RDONLY);
   if(fd == -1){
      syslog(LOG_PRIORITY, "%s: cannot open", tmpfile);
      return 0;
   }

   while((n = read(fd, buf, MAXBUFSIZE)) > 0){
      send(psd, buf, n, 0);
   }

   close(fd);

   /* 
    * read answer from spamd. We do not want to rewrite the message,
    * just to determine whether it's a spam or not.
    */

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);

   p = strstr(buf, "\r\n\r\n");
   if(p){
      *p = '\0';
      p = strstr(buf, SPAMD_RESP_SPAM_STRING);
      if(p){
         rc = 1;
         syslog(LOG_PRIORITY, "%s: %s", tmpfile, p+SPAMD_RESP_SPAM_STRING_LENGTH);
      }
   }

   close(psd);


   return rc;
}

