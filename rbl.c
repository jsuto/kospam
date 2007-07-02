/*
 * rbl.c, 2006.04.05, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "config.h"

int rbl_check(char *rbldomain, char *host){
   char domainname[SMALLBUFSIZE];
   struct hostent *h;

   snprintf(domainname, SMALLBUFSIZE-1, "%s.%s", host, rbldomain);

#ifdef DEBUG
   fprintf(stderr, "SURBL checking: %s\n", domainname);
#endif

   h = gethostbyname(domainname);
   if(h)
      return 1;

   return 0;
}
