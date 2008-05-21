/*
 * ooop-util.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <limits.h>
#include "config.h"


int recvtimeoutssl(int s, char *buf, int len, int timeout, int use_ssl, SSL *ssl){
    fd_set fds;
    int n;
    struct timeval tv;

    memset(buf, 0, MAXBUFSIZE);

    FD_ZERO(&fds);
    FD_SET(s, &fds);

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    if(use_ssl == 1)
       return SSL_read(ssl, buf, len);
    else
       return recv(s, buf, len, 0);
}


int write1(int sd, char *buf, int use_ssl, SSL *ssl){
   int err;

   if(use_ssl == 1)
      err = SSL_write(ssl, buf, strlen(buf));
   else
      err = send(sd, buf, strlen(buf), 0);

   return err;
}


void drop_root(int uid, int gid){
   if(gid > 0 && gid < 65535)
      setgid(gid);

   if(uid > 0 && uid < 65535)
      setuid(uid);
}

