/*
 * rbl.c, 2008.03.05, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "misc.h"
#include "config.h"


/*
 * rbl check the given host against an rbl domain
 */

int rbl_check(char *rbldomain, char *host){
   char domainname[SMALLBUFSIZE];
   struct hostent *h;

   snprintf(domainname, SMALLBUFSIZE-1, "%s.%s", host, rbldomain);

#ifdef DEBUG
   fprintf(stderr, "RBL checking: %s\n", domainname);
#endif

   h = gethostbyname(domainname);
   if(h)
      return 1;

   return 0;
}


/*
 * reverse the given IPv4 address
 */

int reverse_ipv4_addr(char *ip){
   struct in_addr addr;

   if(inet_aton(ip, &addr)){
      addr.s_addr = ntohl(addr.s_addr);
      strcpy(ip, inet_ntoa(addr));

      return 1;
   }

   return 0;
}


/*
 * roll the given host through a comma separated domain list
 */

int rbl_list_check(char *domainlist, char *hostlist){
   char *p, *q, rbldomain[MAX_TOKEN_LEN], host[2*MAX_TOKEN_LEN];

   if(strlen(domainlist) < 3 || strlen(hostlist) < 3) return 0;

   p = domainlist;
   do {
      p = split(p, ',', rbldomain, MAX_TOKEN_LEN-1);

      q = hostlist;
      do {
         q = split(q, ',', host, 2*MAX_TOKEN_LEN-1);
         if(strlen(host) > 5){
            reverse_ipv4_addr(host);
            if(rbl_check(rbldomain, host) == 1)
               return 1;
         }

      } while(q);

   } while(p);

   return 0;
}



