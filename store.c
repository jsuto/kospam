/*
 * store.c, 2008.03.04, SJ
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
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "misc.h"
#include "clapfstore.h"
#include "ssl_util.h"
#include "cfg.h"
#include "config.h"


#ifdef HAVE_SSL
   #include <openssl/crypto.h>
   #include <openssl/x509.h>
   #include <openssl/pem.h>
   #include <openssl/ssl.h>
   #include <openssl/err.h>
   #include "ssl_util.h"
#endif

store *store_init(char *store_addr, int store_port){
   struct sockaddr_in remote_addr;
   struct in_addr addr;
   char buf[MAXBUFSIZE];
   store *STORE;
#ifdef HAVE_SSL
   SSL_METHOD *meth2 = NULL;
#endif

   if(store_addr == NULL || store_port < 1) return NULL;

   STORE = malloc(sizeof(store));
   if(STORE == NULL) return NULL;

#ifdef HAVE_SSL
   STORE->ctx = NULL;
   STORE->ssl = NULL;
#endif
   STORE->fd = -1;
   STORE->rc = 0;

   if((STORE->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      //printf("cannot create socket to store\n");
      goto CLOSE;
   }

   remote_addr.sin_family = AF_INET;
   remote_addr.sin_port = htons(store_port);
   inet_aton(store_addr, &addr);
   remote_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(remote_addr.sin_zero), 8);


   /* init SSL stuff */

   #ifdef HAVE_SSL
      SSL_load_error_strings();
      SSLeay_add_ssl_algorithms();
      meth2 = TLSv1_client_method();

      STORE->ctx = SSL_CTX_new(meth2);
      if(STORE->ctx == NULL) goto CLOSE;

      STORE->ssl = SSL_new(STORE->ctx);
      if(STORE->ssl == NULL) goto CLOSE;

      if(SSL_set_fd(STORE->ssl, STORE->fd) != 1) goto CLOSE;
   #endif

   if(connect(STORE->fd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1){
      goto CLOSE;
   }


   #ifdef HAVE_SSL
      if(SSL_connect(STORE->ssl) == -1) goto CLOSE;
   #endif

   /* read banner */

#ifdef HAVE_SSL
   if(read1(STORE->fd, buf, 1, STORE->ssl) < 1){
#else
   if(recvtimeout(STORE->fd, buf, MAXBUFSIZE, 0) < 1){
#endif
      goto CLOSE;
   }

   if(strncmp(buf, "+OK", 3) == 0){
      STORE->rc = 1;
   }


CLOSE:
   return STORE;
}


int store_email(store *st, char *filename, char *user, char *sig, int ham_or_spam, char *secret){
   int rc = 0;
   char buf[MAXBUFSIZE];
   FILE *f;

   snprintf(buf, MAXBUFSIZE-1, "STOR %s %s %d %s\r\n", user, sig, ham_or_spam, secret);
#ifdef HAVE_SSL
   SSL_write(st->ssl, buf, strlen(buf));
#else
   send(st->fd, buf, strlen(buf), 0);
#endif

   /* read authentication result */

#ifdef HAVE_SSL
   if(read1(st->fd, buf, 1, st->ssl) < 1){
#else
   if(recvtimeout(st->fd, buf, MAXBUFSIZE, 0) < 1){
#endif
      //printf("cannot read store answer after auth\n");
      goto CLOSE_STORE;
   }

   if(strncmp(buf, "+OK", 3)){
      //printf("authentication failure\n");
      goto CLOSE_STORE;
   }


   /* send the email */

   f = fopen(filename, "r");
   if(f){
      while(fgets(buf, MAXBUFSIZE-1, f)){
      #ifdef HAVE_SSL
         SSL_write(st->ssl, buf, strlen(buf));
      #else
         send(st->fd, buf, strlen(buf), 0);
      #endif
      }

      fclose(f);
   }

   /* send ending PERIOD (.) */

#ifdef HAVE_SSL
   SSL_write(st->ssl, "\r\n.\r\n", 5);
   read1(st->fd, buf, 1, st->ssl);
#else
   send(st->fd, "\r\n.\r\n", 5, 0);
   recvtimeout(st->fd, buf, MAXBUFSIZE, 0);
#endif

   if(strncmp(buf, "+OK", 3) == 0){
      rc = 1; 
   }

CLOSE_STORE:

   return rc;
}


int retr_email(store *st, char *filename, char *user, char *sig, int ham_or_spam, char *secret){
   int i=0, n, rc=0, prevlen=0, tot_len=0;
   char buf[MAXBUFSIZE], cmdbuf[MAXBUFSIZE], puf[2*MAXBUFSIZE];

   snprintf(buf, MAXBUFSIZE-1, "RETR %s %s %d %s\r\n", user, sig, ham_or_spam, secret);
#ifdef HAVE_SSL
   SSL_write(st->ssl, buf, strlen(buf));
#else
   send(st->fd, buf, strlen(buf), 0);
#endif

   /* read the email */

#ifdef HAVE_SSL
   while((n = read1(st->fd, buf, 1, st->ssl))){
#else
   while((n = recvtimeout(st->fd, buf, MAXBUFSIZE, 0))){
#endif
      if(i == 0){
         if(strncmp(buf, "+OK", 3)){
            printf("missing file or authentication failure\n");
            goto CLOSE_STORE;
         }
         else rc = 1;
      }

      printf("%s", buf);

      tot_len += n;

      memset(puf, 0, 2*MAXBUFSIZE);
      memcpy(puf, cmdbuf, prevlen);
      memcpy(puf+prevlen, buf, n);

      // save buffer
      memcpy(cmdbuf, buf, n);

      if(puf[prevlen+n-5] == '\r' && puf[prevlen+n-4] == '\n' && puf[prevlen+n-3] == '.' && puf[prevlen+n-2] == '\r' && puf[prevlen+n-1] == '\n') break;

      prevlen = n;

      memset(buf, 0, MAXBUFSIZE);
      i++;
   }

CLOSE_STORE:

   return rc;
}


void store_free(store *st){
#ifdef HAVE_SSL
   SSL_free(st->ssl);
   SSL_CTX_free(st->ctx);
#endif

   close(st->fd);

   free(st);
}

