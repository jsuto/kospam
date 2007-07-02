/*
 * parsembox.c, 2007.05.24, SJ
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
   char buf[MAXBUFSIZE], ifile[SMALLBUFSIZE], *p;
   char year[4];
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

         From email Tue Nov  2 09:28:40 2004
         From MAILER-DAEMON Tue Nov  2 09:33:21 2004

         Thunderbird starts with this: From - Thu Sep 15 11:27:01 2005
      */

      if(buf[0] == 'F' && buf[1] == 'r' && buf[2] == 'o' && buf[3] == 'm' && buf[4] == ' '){
         if(strncmp("From MAILER-DAEMON", buf, 18) == 0 || strchr(buf, '@') || strncmp("From - ", buf, 7) == 0){

            p = strchr(buf+5, ' ');
            if(!p) continue;

            p++;

            buf[strlen(buf)-1] = '\0';

            /* p: Tue Nov  2 09:28:40 2004 */

            if(isupper(*p) && isupper(*(p+4)) &&
                  *(p+3) == ' ' && *(p+7) == ' ' && *(p+10) == ' ' && *(p+19) == ' ' && *(p+13) == ':' && *(p+16) == ':'){

               memcpy(year, p+20, 4);
               if(atoi(year) > 100){
                  is_match = 1;
               }
            }
         }
      }

      if(is_match == 1){
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
