/*
 * parsembox.c, 2007.09.25, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "misc.h"
#include "decoder.h"
#include "parser.h"
#include "config.h"

int main(int argc, char **argv){
   struct _state state;
   int is_match, tot_msgs = 0;
   char buf[MAXBUFSIZE], ifile[SMALLBUFSIZE];
   FILE *F;

   if(argc < 2)
      __fatal("usage: <mbox file>");


   F = fopen(argv[1], "r");
   if(!F)
      __fatal("open");

   state = init_state();

   while(fgets(buf, MAXBUFSIZE-1, F)){
      is_match = 0;

      /*
         a message starts in an mbox file like this:

         From_

         where _ means the space character

      */

      if(buf[0] == 'F' && buf[1] == 'r' && buf[2] == 'o' && buf[3] == 'm' && buf[4] == ' '){
         tot_msgs++;

         if(state.first){
            if(state.num_of_images > 0 || state.num_of_msword > 0){
               snprintf(ifile, SMALLBUFSIZE-1, "%s", state.attachedfile);
               unlink(ifile);
            }
            free_and_print_list(state.first, 1);
         }

         state = init_state();

         printf("*** NEW_MSG_STARTS_HERE %d ***\n", tot_msgs);
         continue;
      }

      state = parse(buf, state);

   }

   /* free the last message */

   if(state.first){
      if(state.num_of_images > 0 || state.num_of_msword > 0){
         snprintf(ifile, SMALLBUFSIZE-1, "%s", state.attachedfile);
         unlink(ifile);
      }

      free_and_print_list(state.first, 1);
   }

   fclose(F);

   return 0;
}
