/*
 * digest.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <clapf.h>
#include <openssl/evp.h>


void digest_file(char *filename, char *digest){
   int fd, i, n;
   unsigned char buf[MAXBUFSIZE], md[DIGEST_LENGTH];
   SHA256_CTX context;

   memset(digest, 0, 2*DIGEST_LENGTH+1);

   fd = open(filename, O_RDONLY);
   if(fd == -1) return;

   SHA256_Init(&context);

   while((n = read(fd, buf, sizeof(buf))) > 0){
      SHA256_Update(&context, buf, n);
   }

   close(fd);

   SHA256_Final(md, &context);

   for(i=0;i<DIGEST_LENGTH;i++)
      snprintf(digest + i*2, 2*DIGEST_LENGTH, "%02x", md[i]);

}

