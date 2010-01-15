/*
 * parsembox.c, 2010.01.15, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#include <sysexits.h>
#include <clapf.h>
#include <locale.h>

int main(int argc, char **argv){
   struct _state state;
   struct session_data sdata;
   struct __config cfg;
   int is_match, tot_msgs = 0;
   char *configfile=CONFIG_FILE, buf[MAXBUFSIZE], ifile[SMALLBUFSIZE];
   FILE *F;

   cfg = read_config(configfile);

   setlocale(LC_MESSAGES, cfg.locale);
   setlocale(LC_CTYPE, cfg.locale);

   if(argc < 2)
      __fatal("usage: <mbox file>");


   F = fopen(argv[1], "r");
   if(!F)
      __fatal("open");

   init_state(&state);

   while(fgets(buf, MAXBUFSIZE-1, F)){
      is_match = 0;

      /*
         a message starts in an mbox file like this:

         From_

         where _ means the space character

      */

      if(buf[0] == 'F' && buf[1] == 'r' && buf[2] == 'o' && buf[3] == 'm' && buf[4] == ' '){
         tot_msgs++;

         if(counthash(state.token_hash) > 0){
            if(state.num_of_images > 0 || state.num_of_msword > 0){
               snprintf(ifile, SMALLBUFSIZE-1, "%s", state.attachedfile);
               unlink(ifile);
            }

            free_list(state.urls);
            clearhash(state.token_hash, 2);
            free_boundary(state.boundaries);
         }

         init_state(&state);

         printf("*** NEW_MSG_STARTS_HERE %d ***\n", tot_msgs);
         continue;
      }

      parse(buf, &state, &sdata, &cfg);

   }

   /* free the last message */

   if(counthash(state.token_hash) > 0){
      if(state.num_of_images > 0 || state.num_of_msword > 0){
         snprintf(ifile, SMALLBUFSIZE-1, "%s", state.attachedfile);
         unlink(ifile);
      }

      free_list(state.urls);
      clearhash(state.token_hash, 2);
      free_boundary(state.boundaries);
   }

   fclose(F);

   return 0;
}
