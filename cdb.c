/*
 * cdb.c, 2007.05.30, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cdb.h>
#include "cdb1.h"
#include "misc.h"
#include "config.h"


/*
 * init cdb stuff
 */

int init_cdbs(char *tokensfile){
   int cfd;

   if(tokensfile){
      cfd = open(tokensfile, O_RDONLY);
      if(cfd != -1){
         if(cdb_init(&tokenscdb, cfd) < 0){
            close(cfd);
            cfd = -1;
         }
         return 1;
      }
   }

   return 0;
}


/*
 * release cdb stuff
 */

void close_cdbs(){
   int cfd;

   cfd = cdb_fileno(&tokenscdb);
   if(cfd != -1)
      close(cfd);

   cdb_free(&tokenscdb);
}


/*
 * query the spamicity value of a token
 */

float cdbqry(struct cdb CDB, char *s){
   float r = DEFAULT_SPAMICITY;
   unsigned int vlen, vpos;
   char res[MAXBUFSIZE];

   memset(res, 0, MAXBUFSIZE);

#ifdef HAVE_NO_64_HASH
   if(cdb_find(&CDB, s, strlen(s)) > 0){
#else
   char x[SMALLBUFSIZE];
   snprintf(x, SMALLBUFSIZE-1, "%llu", APHash(s));

   if(cdb_find(&CDB, x, strlen(x)) > 0){
#endif
       vpos = cdb_datapos(&CDB);
       vlen = cdb_datalen(&CDB);
       if(vlen < MAXBUFSIZE){
          cdb_read(&CDB, &res[0], vlen, vpos);
          r = atof(res);
       }
   }

   return r;
}

