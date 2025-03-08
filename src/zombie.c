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


void zombie_init(struct __data *data, struct __config *cfg){
   int i;
   char buf[SMALLBUFSIZE];
   FILE *f;

   if(data->n_regex > 0){
      for(i=0; i < data->n_regex; i++){
         regfree(&(data->pregs[i]));
      }
   }

   data->n_regex = 0;

   i=0;
   f = fopen(ZOMBIE_NET_REGEX, "r");
   if(f){
      while(fgets(buf, sizeof(buf)-1, f)){
         if(buf[0] != ';' && buf[0] != '#' && buf[0] != '\r' && buf[0] != '\n'){
            trim_buffer(buf);
            if(regcomp(&(data->pregs[data->n_regex]), buf, REG_ICASE | REG_EXTENDED) == 0){
               i++;
               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "[%d] compiled: %s", i, buf);
               data->n_regex++;
            }
            else
               syslog(LOG_PRIORITY, "error: failed to compile '%s'", buf);
         }

         if(data->n_regex == NUM_OF_REGEXES-1) break;

      }
      fclose(f);
   }
   else syslog(LOG_PRIORITY, "error: cannot open '%s'", ZOMBIE_NET_REGEX);
}


void check_zombie_sender(struct session_data *sdata, struct __data *data, struct __config *cfg){
   int i=0;
   size_t nmatch=0;

   while(i < data->n_regex && sdata->tre != '+'){
      if(regexec(&(data->pregs[i]), sdata->hostname, nmatch, NULL, 0) == 0){
         sdata->tre = '+';
      }

      i++;
   }

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: zombie check: %c [%d] %s", sdata->ttmpfile, sdata->tre, i, sdata->hostname);
}


void zombie_free(struct __data *data){
   int i;

   for(i=0; i<data->n_regex; i++){
      regfree(&(data->pregs[i]));
   }
}

