/*
 * clapfconf.c, 2010.01.11, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <clapf.h>

extern char *optarg;
extern int optind;


int main(int argc, char **argv){
   int i, print_from_file=0;
   char *configfile=CONFIG_FILE;
   struct __config cfg;

   while((i = getopt(argc, argv, "c:nh?")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'n' :
                    print_from_file = 1;
                    break;

         case 'h' :
         case '?' :
                    printf("clapfconf [-n -c <configfile>]\n");
                    break;


         default  : 
                    break;
       }
   }


   cfg = read_config(configfile);

   if(print_from_file == 1){
      print_config(configfile, &cfg);
   }
   else {
      print_config_all(&cfg);
   }

   return 0;
}
