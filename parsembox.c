/*
 * parsembox.c, 2009.01.07, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "misc.h"
#include "decoder.h"
#include "list.h"
#include "parser.h"
#include "cfg.h"
#include "hash.h"
#include "config.h"

int main(int argc, char **argv){
   struct _state state;
   struct session_data sdata;
   struct __config cfg;
   int is_match, tot_msgs = 0;
   char buf[MAXBUFSIZE], ifile[SMALLBUFSIZE];
   FILE *F;

   cfg = read_config(NULL);

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

            free_url_list(state.urls);
            clearhash(state.token_hash, 1);
         }

         init_state(&state);

         printf("*** NEW_MSG_STARTS_HERE %d ***\n", tot_msgs);
         continue;
      }

      parse(buf, &state, &sdata, cfg);

   }

   /* free the last message */

   if(counthash(state.token_hash) > 0){
      if(state.num_of_images > 0 || state.num_of_msword > 0){
         snprintf(ifile, SMALLBUFSIZE-1, "%s", state.attachedfile);
         unlink(ifile);
      }

      free_url_list(state.urls);
      clearhash(state.token_hash, 1);
   }

   fclose(F);

   return 0;
}
