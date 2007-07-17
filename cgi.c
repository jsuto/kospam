/*
 * cgi.c, 2007.07.16, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "config.h"


void errout(char *input, char *s){
   if(input)
      free(input);

   input = NULL;

   printf("%s\n", s);

   exit(1);
}


void create_ham_or_spam_path(char *dir, char *name, char *what){
   struct timezone tz;
   struct timeval tv;
   struct tm *t;
   time_t clock;

   gettimeofday(&tv, &tz);
   t = localtime(&clock);

   snprintf(name, SMALLBUFSIZE-1, "%s/%d%02d%02d%02d%02d%02d.%ld-%s", dir, t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec, what);
}


void create_yyyymmddhhmmss(char *name){
   struct timezone tz;
   struct timeval tv;
   struct tm *t;
   time_t clock;

   gettimeofday(&tv, &tz);
   t = localtime(&clock);

   snprintf(name, SMALLBUFSIZE-1, "%d%02d%02d%02d%02d%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
}
