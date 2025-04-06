/*
 * digest.c, SJ
 */

#include <kospam.h>
#include <openssl/evp.h>


void digest_string(char *digestname, char *s, char *digest){
   EVP_MD_CTX *ctx;
   const EVP_MD *md;
   unsigned int i, md_len;
   unsigned char md_value[DIGEST_LENGTH];

   memset(digest, 0, 2*DIGEST_LENGTH+2);

   md = EVP_get_digestbyname(digestname);
   if(md == NULL){
      syslog(LOG_PRIORITY, "ERROR: unknown message digest: '%s' in %s:%d", digestname, __func__, __LINE__);
      return;
   }

   ctx = EVP_MD_CTX_new();
   EVP_DigestInit_ex(ctx, md, NULL);
   EVP_DigestUpdate(ctx, s, strlen(s));
   EVP_DigestFinal_ex(ctx, md_value, &md_len);
   EVP_MD_CTX_free(ctx);

   for(i=0;i<md_len;i++){
      snprintf(digest + i*2, 2*DIGEST_LENGTH, "%02x", md_value[i]);
   }
}
