/*
 * aphash.c, 2010.05.13, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "messages.h"
#include "config.h"

int main(int argc, char **argv){
   FILE *f;
   char *p, buf[MAXBUFSIZE], puf[MAXBUFSIZE];
   unsigned long long hash;
   int len;

   if(argc < 2){
      f = fopen("/dev/stdin", "r");
      if(!f)
         __fatal(ERR_CANNOT_OPEN);

      while(fgets(buf, MAXBUFSIZE-1, f)){
         len = 0;
         snprintf(puf, MAXBUFSIZE-1, "%s", buf);
         p = strrchr(puf, ' ');
         if(p){
            *p = '\0';
            p = strrchr(puf, ' ');
            if(p){
               *p = '\0';
               p = strrchr(puf, ' ');
               if(p){
                  *p = '\0';
               #ifdef HAVE_MYSQL
                  p = strrchr(puf, ' ');
                  if(p){
                     *p = '\0';
               #endif
                     len = strlen(puf);
               #ifdef HAVE_MYSQL
                  }
               #endif
               }
            }
         }
         if(len > 1){
            p = buf + len;

            buf[len] = '\0';
            hash = APHash(buf);
            p++;
            if(hash > 0) printf("%llu %s", hash, p);
         }
      }
      fclose(f);
   }
   else {
      printf("%s %llu\n", argv[1], APHash(argv[1]));
   }

   return 0;
}

