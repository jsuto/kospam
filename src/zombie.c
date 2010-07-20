/*
 * zombie.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <syslog.h>
#include <unistd.h>
#include <clapf.h>


void initialiseZombieList(struct __data *data, struct __config *cfg){
   int i;
   char buf[SMALLBUFSIZE];
   FILE *f;

   if(data->n_regex > 0){
      for(i=0; i < data->n_regex; i++){
         regfree(&(data->pregs[i]));
      }
   }

   data->n_regex = 0;

   f = fopen(ZOMBIE_NET_REGEX, "r");
   if(f){
      while(fgets(buf, SMALLBUFSIZE, f)){
         if(buf[0] != ';' && buf[0] != '#' && buf[0] != '\r' && buf[0] != '\n'){
            trimBuffer(buf);
            if(regcomp(&(data->pregs[data->n_regex]), buf, REG_ICASE | REG_EXTENDED) == 0){
               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "compiled: %s", buf);
               data->n_regex++;
            }
            else
               syslog(LOG_PRIORITY, "failed to compile: %s", buf);
         }

         if(data->n_regex == NUM_OF_REGEXES-1) break;

      }
      fclose(f);
   }
   else syslog(LOG_PRIORITY, "cannot open: %s", ZOMBIE_NET_REGEX);
}


void freeZombieList(struct __data *data){
   int i;

   for(i=0; i<data->n_regex; i++){
      regfree(&(data->pregs[i]));
   }
}

