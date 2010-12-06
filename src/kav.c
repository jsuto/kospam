/*
 * kav.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <clapf.h>

int kav_scan(char *tmpfile, char *engine, char *avinfo, struct __config *cfg){
   int s, n;
   char *p, buf[MAXBUFSIZE], scan_cmd[SMALLBUFSIZE];
   struct sockaddr_un server;

   memset(avinfo, 0, SMALLBUFSIZE);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "trying to pass to KAV: %s", tmpfile);

   strcpy(server.sun_path, cfg->kav_socket);
   server.sun_family = AF_UNIX;

   if((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
      syslog(LOG_PRIORITY, "ERR: create socket");
      return AV_ERROR;
   }

   if(connect(s, (struct sockaddr *)&server, strlen(server.sun_path) + sizeof (server.sun_family)) == -1){
      syslog(LOG_PRIORITY, "KAV ERR: connect to %s", cfg->kav_socket);
      return AV_ERROR;
   }

   /* read KAV banner. It should start with KAV_READY */

   n = recvtimeout(s, buf, MAXBUFSIZE, TIMEOUT);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "KAV got: %s", buf);

   if(strncmp(buf, KAV_READY, 4)){
      send(s, KAV_CMD_QUIT, strlen(KAV_CMD_QUIT), 0);
      close(s);
      syslog(LOG_PRIORITY, "KAV ERR: missing '201 Ready' banner");
      return AV_ERROR;
   }

   /* issue the SCAN command with full path to the temporary directory */

   memset(scan_cmd, 0, SMALLBUFSIZE);
   snprintf(scan_cmd, SMALLBUFSIZE-1, "SCAN xmQPRSTUWabcdefghi %s/%s\r\n", cfg->workdir, tmpfile);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "KAV CMD: %s", scan_cmd);

   send(s, scan_cmd, strlen(scan_cmd), 0);

   /* read KAV's answers */

   while((n = recvtimeout(s, buf, MAXBUFSIZE, TIMEOUT)) > 0){
      //buf[n] = '\0';
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "KAV DEBUG: %d %s", n, buf);

      if(strncmp(buf, KAV_RESP_INFECTED_NAME, strlen(KAV_RESP_INFECTED_NAME)) == 0){
         p = strchr(buf, ' ');
         if(p)
            *p = '\0';

         strncpy(avinfo, buf, SMALLBUFSIZE-1);
      }

      if( (buf[0] == '2' || buf[0] == '5') && buf[3] == ' ')
         break;
   }


   send(s, KAV_CMD_QUIT, strlen(KAV_CMD_QUIT), 0);
   close(s);

   if(strncmp(buf, KAV_RESP_INFECTED, strlen(KAV_RESP_INFECTED)) == 0){
      snprintf(engine, SMALLBUFSIZE-1, "Kaspersky");
      return AV_VIRUS;
   }

   return AV_OK;
}
