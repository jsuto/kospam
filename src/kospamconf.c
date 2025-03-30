/*
 * clapfconf.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <clapf.h>

extern char *optarg;
extern int optind;


void print_config_all(struct __config *cfg, char *key);
void print_config(char *configfile, struct __config *cfg);

int main(int argc, char **argv){
   int i, print_from_file=0;
   char *configfile=CONFIG_FILE, *query=NULL;
   struct __config cfg;

   while((i = getopt(argc, argv, "c:q:nh?")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'n' :
                    print_from_file = 1;
                    break;

         case 'q' :
                    query = optarg;
                    break;

         case 'h' :
         case '?' :
                    printf("clapfconf [-n -c <configfile>] [-q <key>]\n");
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
      print_config_all(&cfg, query);
   }

   return 0;
}
