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
#include <math.h>
#include <clapf.h>


int is_host_on_rbl(char *host, char *domain){
   int rc=0;
   char *p, domainname[SMALLBUFSIZE];
   struct addrinfo hints, *res;

   p = strchr(host, ' '); if(p) *p = '\0';

   snprintf(domainname, sizeof(domainname)-1, "%s.%s", host, domain);

   if(getaddrinfo(domainname, "25", &hints, &res) == 0){
      freeaddrinfo(res);
      rc = 1;
   }

   return rc;
}


int is_host_on_rbl_lists(char *host, char *domainlist){
   int result;
   char *p, domain[MAXVAL];

   if(strlen(domainlist) < 3 || strlen(host) < 3) return 0;

   p = domainlist;
   do {
      p = split(p, ' ', domain, sizeof(domain)-1, &result);

      if(is_host_on_rbl(host, domain) == 1) return 1;

   } while(p);

   return 0;
}


int check_rbl_lists(struct __state *state, char *domainlist, struct __config *cfg){
   int i, rc=0;
   char rbl_token[MAXVAL];
   struct node *q;
   struct timezone tz;
   struct timeval tv1, tv2;

   /* consult URL blacklists */

   if(!domainlist) return rc;

   gettimeofday(&tv1, &tz);

   for(i=0;i<MAXHASH;i++){
      q = state->url[i];
      while(q != NULL){

         if(count_character_in_buffer(q->str+4, '.') > 0){
            if(is_host_on_rbl_lists(q->str+4, domainlist) > 0){
               rc = 1;
               snprintf(rbl_token, sizeof(rbl_token)-1, "SURBL*%s", (char*)(q->str)+4);
               addnode(state->token_hash, rbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            }
         }

         q = q->r;
      }
   }

   gettimeofday(&tv2, &tz);

   return rc;
}

