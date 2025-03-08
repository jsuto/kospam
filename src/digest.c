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


void raw_digest_file(char *digestname, char *filename, unsigned char *md_value){
   int fd, n;
   unsigned char buf[MAXBUFSIZE];
   EVP_MD_CTX *ctx;
   const EVP_MD *md;
   unsigned int md_len;

   md = EVP_get_digestbyname(digestname);
   if(md == NULL){
      syslog(LOG_PRIORITY, "ERROR: unknown message digest: '%s' in %s:%d", digestname, __func__, __LINE__);
      return;
   }


#ifdef DEBUG
   syslog(LOG_PRIORITY, "open '%s' in %s:%d", filename, __func__, __LINE__);
#endif
   fd = open(filename, O_RDONLY);
   if(fd == -1) return;

   ctx = EVP_MD_CTX_new();
   EVP_DigestInit_ex(ctx, md, NULL);

   while((n = read(fd, buf, sizeof(buf))) > 0){
      EVP_DigestUpdate(ctx, buf, n);
   }

#ifdef DEBUG
   syslog(LOG_PRIORITY, "close '%s' in %s:%d", filename, __func__, __LINE__);
#endif
   close(fd);

   EVP_DigestFinal_ex(ctx, md_value, &md_len);
   EVP_MD_CTX_free(ctx);
}

void digest_file(char *filename, char *digest){
   unsigned char md[DIGEST_LENGTH];

   raw_digest_file("sha256", filename, &md[0]);

   memset(digest, 0, 2*DIGEST_LENGTH+1);

   for(int i=0;i<SHA256_DIGEST_LENGTH;i++){
      snprintf(digest + i*2, 2*DIGEST_LENGTH, "%02x", md[i]);
   }
}
