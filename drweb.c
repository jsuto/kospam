/*
 * drweb.c, 2006.02.28, SJ
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "config.h"
#include "misc.h"
#include "drweb.h"

int drweb_scan(char *drweb_socket, char *tmpfile, int v, char *drwebinfo){
   int sd, n, fd;
   unsigned long q, size;
   unsigned char buf[MAXBUFSIZE];
   struct sockaddr_un server;
   struct stat st;
   struct timezone tz;
   struct timeval tv_start, tv_sent;

   memset(drwebinfo, 0, SMALLBUFSIZE);

   if(v >= _LOG_DEBUG) syslog(LOG_PRIORITY, "trying to pass to Dr.Web: %s", tmpfile);
   gettimeofday(&tv_start, &tz);

   strcpy(server.sun_path, drweb_socket);
   server.sun_family = AF_UNIX;

   if((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
      syslog(LOG_PRIORITY, "ERR: create socket");
      return DRWEB_ERROR;
   }

   if(connect(sd, (struct sockaddr *)&server, strlen(server.sun_path) + sizeof (server.sun_family)) == -1){
      syslog(LOG_PRIORITY, "DRWEB ERR: connect to %s", drweb_socket);
      return DRWEB_ERROR;
   }

   size = 0;

   if(stat(tmpfile, &st) == 0)
      size = st.st_size;

   if(size <= 0){
      syslog(LOG_PRIORITY, "DRWEB ERR: invalid size of %s %ld", tmpfile, size); 
      return DRWEB_ERROR;
   }

   fd = open(tmpfile, O_RDONLY);
   if(fd == -1){
      syslog(LOG_PRIORITY, "DRWEB ERR: cannot open %s", tmpfile);
      return DRWEB_ERROR;
   }

   memset(buf, 0, MAXBUFSIZE);

   buf[3] = 1;
   buf[4] = 128;
   buf[15] = 30;

   memcpy(&buf[16], tmpfile, 30);

   // size of message

   q = htonl(size);
   memcpy(&buf[46], (unsigned char*)&q, 4);

   send(sd, buf, 50, 0);

   while((n = read(fd, buf, MAXBUFSIZE)) > 0){
      send(sd, buf, n, 0); 
   }
   close(fd);      

   n = recv(sd, (unsigned char*)&q, 4, 0);
   close(sd);

   gettimeofday(&tv_sent, &tz);

   syslog(LOG_PRIORITY, "%s: scanned %ld [ms]", tmpfile, tvdiff(tv_sent, tv_start)/1000);

   if(ntohl(q) == DRWEB_RESP_VIRUS){
      strncpy(drwebinfo, DRWEB_VIRUS_HAS_FOUND_MESSAGE, SMALLBUFSIZE-1);
      return DRWEB_VIRUS;
   }

   return DRWEB_OK;
}

