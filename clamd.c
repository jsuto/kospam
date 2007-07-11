/*
 * clamd.c, 2006.03.09, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "misc.h"
#include "clamd.h"
#include "config.h"

int clamd_scan(char *clamd_socket, char *chrootdir, char *workdir, char *tmpfile, int v, char *clamdinfo){
   int s, n;
   char *p, *q, buf[MAXBUFSIZE], scan_cmd[SMALLBUFSIZE];
   struct sockaddr_un server;
   struct timezone tz;
   struct timeval tv_start, tv_sent;

   memset(clamdinfo, 0, SMALLBUFSIZE);

   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to pass to CLAMD", tmpfile);

   gettimeofday(&tv_start, &tz);

   strcpy(server.sun_path, clamd_socket);
   server.sun_family = AF_UNIX;

   if((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
      syslog(LOG_PRIORITY, "ERR: create socket");
      return CLAMD_ERROR;
   }

   if(connect(s, (struct sockaddr *)&server, strlen(server.sun_path) + sizeof (server.sun_family)) == -1){
      syslog(LOG_PRIORITY, "CLAMD ERR: connect to %s", clamd_socket);
      return CLAMD_ERROR;
   }


   /* issue the SCAN command with full path to the temporary directory */

   
   memset(scan_cmd, 0, SMALLBUFSIZE);
   snprintf(scan_cmd, SMALLBUFSIZE-1, "SCAN %s/%s/%s\r\n", chrootdir, workdir, tmpfile);

   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: CLAMD CMD: %s", tmpfile, scan_cmd);

   send(s, scan_cmd, strlen(scan_cmd), 0);

   /* read CLAMD's answers */

   n = recvtimeout(s, buf, MAXBUFSIZE, 0);

   close(s);

   gettimeofday(&tv_sent, &tz);
   syslog(LOG_PRIORITY, "%s: scanned %ld [ms]", tmpfile, tvdiff(tv_sent, tv_start)/1000);

   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: CLAMD DEBUG: %d %s", tmpfile, n, buf);

   if(str_case_str(buf, CLAMD_RESP_INFECTED)){
      p = strchr(buf, ' ');
      if(p){
         q = strrchr(p, ' ');
         if(q){
            *q = '\0';
            p++;
            strncpy(clamdinfo, p, SMALLBUFSIZE-1);
         }
      }

      return CLAMD_VIRUS;
   }

   return CLAMD_OK;
}
