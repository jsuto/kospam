/*
   templates.c, 2010.05.13, SJ
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <clapf.h>


int createEmailFromTemplate(char *filename, char *msg, char *postmaster, char *recipient, char *sender, char *virus, char *engine){
   FILE *f;
   char buf[SMALLBUFSIZE], puf[SMALLBUFSIZE], *p;

   memset(msg, 0, MAXBUFSIZE);

   f = fopen(filename, "r");
   if(!f) return 0;

   while((fgets(buf, SMALLBUFSIZE-1, f))){
      p = buf;
      trimBuffer(buf);

      do {
         p = split(p, ' ', puf, MAXBUFSIZE-1);
         if(puf[0] >= 65 && puf[0] <= 90){
            if(strcmp(puf, "POSTMASTERADDRESS") == 0) strncat(msg, postmaster, MAXBUFSIZE-1);
            else if(strcmp(puf, "RECIPIENTADDRESS") == 0) strncat(msg, recipient, MAXBUFSIZE-1);
            else if(strcmp(puf, "SENDERADDRESS") == 0) strncat(msg, sender, MAXBUFSIZE-1);
            else if(strcmp(puf, "THEVIRUS") == 0) strncat(msg, virus, MAXBUFSIZE-1);
            else if(strcmp(puf, "ENGINE") == 0) strncat(msg, engine, MAXBUFSIZE-1);

            else strncat(msg, puf, MAXBUFSIZE-1);
         }
         else
            strncat(msg, puf, MAXBUFSIZE-1);

         if(p) strncat(msg, " ", MAXBUFSIZE-1);

      } while(p);

      strncat(msg, "\r\n", MAXBUFSIZE-1);
   }

   fclose(f);

   return 1;
}

