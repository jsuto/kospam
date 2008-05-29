/*
 * cloudmark.c, 2008.05.29, SJ
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cmae_client.h>
#include "config.h"


int cloudmark_score(char *server, int port, char *message, int len){
   unsigned int score=0;
   CMAE_Client_Session SessionOut;


   if(CMAE_Client_Open(&SessionOut, server, port, 0, 0) == 0){
      CMAE_Client_Score(SessionOut, NULL, message, len, &score, NULL, NULL, NULL, NULL);
   }

   CMAE_Client_Close(SessionOut);

   return score;
}
