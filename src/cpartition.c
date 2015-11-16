/*
 * cpartition.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <getopt.h>
#include <syslog.h>
#include <clapf.h>

int main(){
   char *configfile=CONFIG_FILE;
   struct __config cfg;

   (void) openlog("cpartition", LOG_PID, LOG_MAIL);

   cfg = read_config(configfile);

   create_partition(&cfg);
   drop_partition(&cfg);

   return 0;
}
