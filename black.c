/*
 * black.c, 2007.11.06, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include "misc.h"
#include "config.h"


/*
 * lookup the timestamp of this IP-address
 */

unsigned long blackness(char *dir, char *ip, int v){
   char ipfile[SMALLBUFSIZE];
   unsigned long blackhole_timestamp = 0;
   struct timeval tv_spam_start, tv_spam_stop;
   struct timezone tz;
   struct stat st;

   if(strlen(ip) < 7)
      return blackhole_timestamp;

   /* skip localhost and private IP addresses */

   if( strncmp(ip, "127.", 4) && strncmp(ip, "192.168.", 8) && strncmp(ip, "10.", 3) && strncmp(ip, "172.16.", 7) ){

      gettimeofday(&tv_spam_start, &tz);

      snprintf(ipfile, SMALLBUFSIZE-1, "%s/%s", dir, ip);

      if(stat(ipfile, &st) == 0)
         blackhole_timestamp = st.st_atime;

      gettimeofday(&tv_spam_stop, &tz);

   #ifdef DEBUG
      fprintf(stderr, "blackhole check for %s: %ld in %ld [ms]\n", ip, blackhole_timestamp, tvdiff(tv_spam_stop, tv_spam_start)/1000);
   #else
      if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "checking %s in the blackhole list %ld us", ip, tvdiff(tv_spam_stop, tv_spam_start));
   #endif
   }

   return blackhole_timestamp;
}

void put_ip_to_dir(char *dir, char *ip){
   struct timeval tv1, tv2;
   struct timezone tz;
   char ipfile[SMALLBUFSIZE];
   int fd;

   gettimeofday(&tv1, &tz);

   snprintf(ipfile, SMALLBUFSIZE-1, "%s/%s", dir, ip);
   unlink(ipfile);

   fd = open(ipfile, O_RDWR|O_CREAT, S_IRUSR|S_IRGRP|S_IROTH);
   if(fd != -1){
      close(fd);
      gettimeofday(&tv2, &tz);
      syslog(LOG_PRIORITY, "putting %s to blackhole in %ld [us]", ip, tvdiff(tv2, tv1));
   }
   else syslog(LOG_PRIORITY, "failed to put %s to blackhole", ip);

}

