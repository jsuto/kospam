/*
 * rbl.c, SJ
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


int isIPv4AddressOnRBL(char *ipaddr, char *domainlist){
   char *p, rbldomain[MAX_TOKEN_LEN], host[2*MAX_TOKEN_LEN];

   if(strlen(domainlist) < 3 || strlen(ipaddr) < 3) return 0;

   p = domainlist;
   do {
      p = split(p, ',', rbldomain, MAX_TOKEN_LEN-1);

      snprintf(host, 2*MAX_TOKEN_LEN-1, "%s", ipaddr);

      if(reverseIPv4Address(host) == 1){
         if(checkHostOnRBL(host, rbldomain) == 1) return 1;
      }

   } while(p);

   return 0;
}


int isURLOnRBL(char *url, char *domainlist){
   char *p, rbldomain[MAX_TOKEN_LEN], host[2*MAX_TOKEN_LEN];

   if(strlen(domainlist) < 3 || strlen(url) < 3) return 0;

   p = domainlist;
   do {
      p = split(p, ',', rbldomain, MAX_TOKEN_LEN-1);

      snprintf(host, 2*MAX_TOKEN_LEN-1, "%s", url);

      if(checkHostOnRBL(host, rbldomain) == 1) return 1;

   } while(p);

   return 0;
}


int reverseIPv4Address(char *ipaddr){
   struct in_addr addr;

   if(!ipaddr) return 0;

   if(inet_aton(ipaddr, &addr)){
      addr.s_addr = ntohl(addr.s_addr);
      strcpy(ipaddr, inet_ntoa(addr));

      return 1;
   }

   return 0;
}


int checkHostOnRBL(char *host, char *rbldomain){
   char domainname[SMALLBUFSIZE];
   struct hostent *h;

   snprintf(domainname, SMALLBUFSIZE-1, "%s.%s", host, rbldomain);

   h = gethostbyname(domainname);
   if(h)
      return 1;

   return 0;
}


