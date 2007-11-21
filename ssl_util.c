/*
 * ssl_util.c, 2007.11.08, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fcntl.h>
#include <unistd.h>
#include "config.h"


/*
 * SSL aware read
 */

int read1(int sd, char *buf, int use_ssl, SSL *ssl){
   fd_set fds;
   int err;
   struct timeval tv;

   memset(buf, 0, MAXBUFSIZE);

   FD_ZERO(&fds);
   FD_SET(sd, &fds);

   tv.tv_sec = SMAP_TIMEOUT;
   tv.tv_usec = SMAP_TIMEOUT_USEC;

   err = select(sd+1, &fds, NULL, NULL, &tv);
   if(err == 0) return -2; // timeout!
   if(err == -1) return -1; // error

   if(use_ssl == 1)
      err = SSL_read(ssl, buf, MAXBUFSIZE-1);
   else
      err = recv(sd, buf, MAXBUFSIZE-1, 0);

   return err;
}


/*
 * SSL aware write
 */

int write1(int sd, char *buf, int use_ssl, SSL *ssl){
   int err;

   if(use_ssl == 1)
      err = SSL_write(ssl, buf, strlen(buf));
   else
      err = send(sd, buf, strlen(buf), 0);

   return err;
}


void LOG_MESSAGE(char *s){
   struct tm *t;
   struct timezone tz;
   struct timeval tv;

   gettimeofday(&tv, &tz);
   t = localtime(&(tv.tv_sec));

#ifdef WIN32
   printf("msg: %d.%02d.%02d. %02d:%02d:%02d %s", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, s);
   if(!strchr(s, '\n'))
      printf("\n");
#else
   syslog(LOG_PRIORITY, "msg: %d.%02d.%02d. %02d:%02d:%02d %s", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, s);
#endif
}

