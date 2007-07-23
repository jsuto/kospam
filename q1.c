/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "misc.h"

#define PORT 48791 // the port client will be connecting to 
#define QCACHE_SOCKET "/tmp/qcache"

#define MAXDATASIZE 100 // max number of bytes we can get at once 

#define TESTFILE "/home/sj/temp/test.aa"

int sockfd;
struct timezone tz;
struct timeval tv_start, tv_stop;

void run_test(){
   FILE *f;
   char buf[MAXDATASIZE], buf2[MAXDATASIZE];
   int n=0;

   f = fopen(TESTFILE, "r");
   if(!f) return;

   while(fgets(buf2, MAXDATASIZE-1, f)){
      n++;
      snprintf(buf, MAXDATASIZE-1, "SELECT %s", buf2);

      send(sockfd, buf, strlen(buf), 0);
      recv(sockfd, buf, MAXDATASIZE-1, 0);
      //printf("%s", buf);
   }

   fclose(f);
}

int main(int argc, char *argv[])
{
    int numbytes;  
    char buf[MAXDATASIZE];
    struct hostent *he;

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    if ((he=gethostbyname(argv[1])) == NULL) {  // get the host info 
        herror("gethostbyname");
        exit(1);
    }

#ifdef HAVE_TCP
    struct sockaddr_in their_addr;
 
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
#else
    struct sockaddr_un server;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
#endif
        perror("socket");
        exit(1);
    }

#ifdef HAVE_TCP
    their_addr.sin_family = AF_INET;    // host byte order 
    their_addr.sin_port = htons(PORT);  // short, network byte order 
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
#else
   strcpy(server.sun_path, QCACHE_SOCKET);
   server.sun_family = AF_UNIX;
#endif

#ifdef HAVE_TCP
    if (connect(sockfd, (struct sockaddr *)&their_addr,
                                          sizeof their_addr) == -1) {
#else
    if(connect(sockfd, (struct sockaddr *)&server, strlen(server.sun_path) + sizeof (server.sun_family)) == -1){
#endif

        perror("connect");
        exit(1);
    }

    if ((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    printf("banner: %s", buf);

    gettimeofday(&tv_start, &tz);
    run_test();
    gettimeofday(&tv_stop, &tz);

    printf("time; %ld\n", tvdiff(tv_stop, tv_start));

    close(sockfd);

    return 0;
}
