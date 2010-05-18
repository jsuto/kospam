/*
 * zombietest.c, 2010.05.13, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "misc.h"
#include "config.h"


int main(int argc, char **argv){
   struct __data data;

   if(argc < 2){
      printf("usage: %s <FQDN1> <FQDN2> ...\n", argv[0]);
      return -1;
   }


#ifdef HAVE_TRE
   int i=0, j, found=0;
   char buf[SMALLBUFSIZE];
   size_t nmatch=0;
   FILE *f;

   data.n_regex = 0;

   f = fopen(ZOMBIE_NET_REGEX, "r");
   if(f){
      while(fgets(buf, SMALLBUFSIZE, f)){
         if(buf[0] != ';' && buf[0] != '#' && buf[0] != '\r' && buf[0] != '\n'){
            trimBuffer(buf);
            if(regcomp(&(data.pregs[data.n_regex]), buf, REG_ICASE | REG_EXTENDED) == 0){
               printf("compiled (%d): %s\n", i, buf);
               data.n_regex++;
               i++;
            }   
            else 
               syslog(LOG_PRIORITY, "failed to compile: %s", buf);
         }

         if(data.n_regex == NUM_OF_REGEXES-1) break;

      }
      fclose(f);
   }
   else printf("cannot open: %s\n", ZOMBIE_NET_REGEX);

   printf("\n\n");

   for(j=1; j<argc; j++){
      i = 0;
      found = 0;

      while(i < data.n_regex){
         if(regexec(&(data.pregs[i]), argv[j], nmatch, NULL, 0) == 0){
            printf("%s: match at %d\n", argv[j], i);
            found = 1;
         }

         i++;
      }

      if(found == 0) printf("%s: no match\n", argv[j]);

   }

   for(i=0; i<data.n_regex; i++){
      regfree(&(data.pregs[i]));
   }
#endif

   return 0;
}
