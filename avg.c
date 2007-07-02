/*
 * avg.c, 2006.02.08, SJ
 *
 * Please set "processesArchives = 1" in the [AvgCommon] section of /etc/avg.conf
 * to let AVG scan archive files automatically
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
#include "avg.h"

/*
 * connect to AVG and tell it what directory to scan
 */

int avg_scan(char *avg_address, int avg_port, char *workdir, char *tmpdir, char *tmpfile, int v, char *avginfo){
   int n, psd;
   char *p, buf[MAXBUFSIZE+1], scan_cmd[SMALLBUFSIZE];
   struct in_addr addr;
   struct sockaddr_in avg_addr;
   struct timezone tz;
   struct timeval tv_start, tv_sent;

   memset(avginfo, 0, SMALLBUFSIZE);

   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to pass to AVG", tmpfile);

   gettimeofday(&tv_start, &tz);

   if((psd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      syslog(LOG_PRIORITY, "%s: ERR: create socket", tmpfile);
      return AVG_ERROR;
   }

   avg_addr.sin_family = AF_INET;
   avg_addr.sin_port = htons(avg_port);
   inet_aton(avg_address, &addr);
   avg_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(avg_addr.sin_zero), 8);

   if(connect(psd, (struct sockaddr *)&avg_addr, sizeof(struct sockaddr)) == -1){
      syslog(LOG_PRIORITY, "%s: AVG ERR: connect to %s %d", tmpfile, avg_address, avg_port);
      return AVG_ERROR;
   }

   /* read AVG banner. The last line should end with '220 Ready' */

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);
   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: AVG got: %s", tmpfile, buf);

   if(strncmp(buf, "220", 3)){
      send(psd, AVG_CMD_QUIT, strlen(AVG_CMD_QUIT), 0);
      close(psd);
      syslog(LOG_PRIORITY, "%s: AVG ERR: missing '220 Ready' banner", tmpfile);
      return AVG_ERROR;
   }

   /* issue the SCAN command with full path to the temporary directory */

   memset(scan_cmd, 0, SMALLBUFSIZE);
   snprintf(scan_cmd, SMALLBUFSIZE-1, "SCAN %s/%s\r\n", workdir, tmpdir);

   send(psd, scan_cmd, strlen(scan_cmd), 0);

   /* read AVG's answer */

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);
   close(psd);

   gettimeofday(&tv_sent, &tz);

   syslog(LOG_PRIORITY, "%s: scanned %ld [ms]", tmpfile, tvdiff(tv_sent, tv_start)/1000);


   if(strncmp(buf, AVG_RESP_OK, 3) == 0)
      return AVG_OK;

   if(strncmp(buf, AVG_RESP_VIRUS, 3) == 0){
      p = strstr(buf, "identified");
      if(p)
         strncpy(avginfo, p, SMALLBUFSIZE-1);
      else
         strncpy(avginfo, buf, SMALLBUFSIZE-1);

      avginfo[strlen(avginfo)-2] = '\0';

      return AVG_VIRUS;
   }

   if(strncmp(buf, AVG_RESP_NOT_FOUND, 3) == 0)
      return AVG_NOT_FOUND;

   return AVG_ERROR;

}

