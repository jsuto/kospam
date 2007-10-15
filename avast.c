/*
 * avast.c, 2006.02.07, SJ
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "config.h"
#include "misc.h"
#include "avast.h"

/*
 * connect to avast! and tell it what file to scan
 */

int avast_scan(char *avast_address, int avast_port, char *workdir, char *tmpfile, int v, char *avastinfo){
   int n, psd;
   char *p, *q, buf[MAXBUFSIZE], puf[MAXBUFSIZE], scan_cmd[SMALLBUFSIZE];
   struct in_addr addr;
   struct sockaddr_in avast_addr;

   memset(avastinfo, 0, SMALLBUFSIZE);

   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to pass to avast!", tmpfile);

   if((psd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      syslog(LOG_PRIORITY, "%s: ERR: create socket", tmpfile);
      return AVAST_ERROR;
   }

   avast_addr.sin_family = AF_INET;
   avast_addr.sin_port = htons(avast_port);
   inet_aton(avast_address, &addr);
   avast_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(avast_addr.sin_zero), 8);

   if(connect(psd, (struct sockaddr *)&avast_addr, sizeof(struct sockaddr)) == -1){
      syslog(LOG_PRIORITY, "%s: AVAST ERR: connect to %s %d", tmpfile, avast_address, avast_port);
      return AVAST_ERROR;
   }

   /* read AVAST banner. The last line should end with '220 Ready' */

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);
   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: AVAST got: %s", tmpfile, buf);

   if(strncmp(buf, "220", 3)){
      send(psd, AVAST_CMD_QUIT, strlen(AVAST_CMD_QUIT), 0);
      close(psd);
      syslog(LOG_PRIORITY, "%s: AVAST ERR: missing '220 Ready' banner", tmpfile);
      return AVAST_ERROR;
   }

   /* issue the SCAN command with full path to the temporary directory */

   memset(scan_cmd, 0, SMALLBUFSIZE);
   snprintf(scan_cmd, SMALLBUFSIZE-1, "SCAN %s/%s\r\n", workdir, tmpfile);

   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: AVAST CMD: %s", tmpfile, scan_cmd);

   send(psd, scan_cmd, strlen(scan_cmd), 0);

   /* read AVAST's answers */

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);

   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: AVAST DEBUG: %d %s", tmpfile, n, buf);

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);

   /* close the connection */

   send(psd, AVAST_CMD_QUIT, strlen(AVAST_CMD_QUIT), 0);
   close(psd);

   /* now parse what we got from avast! */

   p = buf;

   do {
      p = split(p, '\n', puf, MAXBUFSIZE-1);

      if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: AVAST DEBUG: %s", tmpfile, puf);

      q = strstr(puf, AVAST_RESP_INFECTED);
      if(q){
         q += strlen(AVAST_RESP_INFECTED);
         strncpy(avastinfo, q+1, SMALLBUFSIZE-1);
         avastinfo[strlen(avastinfo)-1] = '\0';
         return AVAST_VIRUS;
      }

   } while(p);

   return AVAST_OK;
}

