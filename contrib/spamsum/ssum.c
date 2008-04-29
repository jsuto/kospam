/*
 * spamsum.c, 2008.04.29, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include "misc.h"
#include "config.h"

int main(int argc, char **argv){
   char *sum;
   int i, flags=0, block_size=0;

   flags |= FLAG_IGNORE_HEADERS;

   if(argc < 2) return 1;

   for(i=1;i<argc;i++){
      sum = spamsum_file(argv[i], flags, block_size);
      printf("%s\n", sum);
      free(sum);
   }

   return 0;
}
