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


void checkLists(struct session_data *sdata, struct _state *state, int *found_on_rbl, int *surbl_match, struct __config *cfg){
   int i, j;
   char surbl_token[MAX_TOKEN_LEN];
   struct url *url;
   struct timezone tz;
   struct timeval tv1, tv2;


   /* consult blacklists about the IPv4 address connecting to us */

   if(strlen(cfg->rbl_domain) > 3){
      gettimeofday(&tv1, &tz);
      *found_on_rbl = isIPv4AddressOnRBL(state->ip, cfg->rbl_domain);
      gettimeofday(&tv2, &tz);

      if(cfg->debug == 1) printf("rbl check took %ld ms\n", tvdiff(tv2, tv1)/1000);
      if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: rbl check took %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

      for(i=0; i<*found_on_rbl; i++){
         snprintf(surbl_token, MAX_TOKEN_LEN-1, "RBL%d*%s", i, state->ip);
         addnode(state->token_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
      }
   }


   /* consult URL blacklists */

   if(state->urls && strlen(cfg->surbl_domain) > 4){
      url = state->urls;

      while(url){
         if(countCharacterInBuffer(url->url_str+4, '.') > 0){
            gettimeofday(&tv1, &tz);
            i = isURLOnRBL(url->url_str+4, cfg->surbl_domain);
            gettimeofday(&tv2, &tz);

            if(cfg->debug == 1) printf("surbl check for %s (%d) took %ld ms\n", url->url_str+4, i, tvdiff(tv2, tv1)/1000);
            if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: surbl check took %ld ms", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);

            *surbl_match += i;

            for(j=0; j<i; j++){
               snprintf(surbl_token, MAX_TOKEN_LEN-1, "SURBL%d*%s", j, url->url_str+4);
               addnode(state->token_hash, surbl_token, REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            }
         }

         url = url->r;
      }
   }

}


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

