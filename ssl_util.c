/*
 * ssl_util.c, 2008.03.04, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include "config.h"


int read1(int s, char *buf, int use_ssl, SSL *ssl){
    fd_set fds;
    int n;
    struct timeval tv;

    memset(buf, 0, MAXBUFSIZE);

    FD_ZERO(&fds);
    FD_SET(s, &fds);

    tv.tv_sec = STORE_TIMEOUT;
    tv.tv_usec = 0;

    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    if(use_ssl == 1)
       return SSL_read(ssl, buf, MAXBUFSIZE);
    else
       return recv(s, buf, MAXBUFSIZE, 0);
}


int write1(int sd, char *buf, int use_ssl, SSL *ssl){
   int err;

   if(use_ssl == 1)
      err = SSL_write(ssl, buf, strlen(buf));
   else
      err = send(sd, buf, strlen(buf), 0);

   return err;
}

