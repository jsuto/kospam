/*
 * splitmbox.c, 2007.06.28, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "misc.h"
#include "decoder.h"
#include "parser.h"
#include "config.h"

int main(int argc, char **argv){
   int tot_msgs = 0;
   char buf[MAXBUFSIZE], *p, fname[MAXBUFSIZE];
   char year[4];
   FILE *F, *f=NULL;

   if(argc < 3)
      __fatal("usage: <mbox file> <messagefile basename>");


   F = fopen(argv[1], "r");
   if(!F)
      __fatal("open");

   while(fgets(buf, MAXBUFSIZE-1, F)){

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
                  tot_msgs++;
                  if(f) fclose(f);
                  printf("parsing message %d ...\n", tot_msgs);
                  snprintf(fname, 200, "%s-%d", argv[2], tot_msgs);
                  f = fopen(fname, "w+");
                  fprintf(f, "%s\r\n", buf);
                  continue;
               }
            }
         }
      }

      trim(buf);
      fprintf(f, "%s\r\n", buf);
   }

   if(f) fclose(f);

   fclose(F);

   return 0;
}
