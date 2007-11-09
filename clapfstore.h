/*
 * clapfstore.h, 2007.11.08, SJ
 */

#ifdef HAVE_SSL
#include <openssl/ssl.h>
#endif

typedef struct {
#ifdef HAVE_SSL
   SSL *ssl;
   SSL_CTX *ctx;
#endif
   int fd, rc;
} store;


#define CLAPFSTORE_PROGRAM_NAME "store"
#define CLAPFSTORE_VERSION "0.1"

#define RESP_CLAPFSTORE_OK_BANNER "+OK " CLAPFSTORE_PROGRAM_NAME " is ready\r\n"
#define RESP_CLAPFSTORE_ERR_BANNER "-ERR " CLAPFSTORE_PROGRAM_NAME " has a problem\r\n"

#define RESP_CLAPFSTORE_OK "+OK\r\n"
#define RESP_CLAPFSTORE_ERR "-ERR\r\n"

#define RESP_CLAPFSTORE_ERR_AUTH_FAILED "-ERR authentication failed\r\n"

#define CMD_STOR 1
#define CMD_RETR 2


store *store_init(char *store_addr, int store_port);
int store_email(store *st, char *filename, char *user, char *sig, int ham_or_spam, char *secret);
int retr_email(store *st, char *filename, char *user, char *sig, int ham_or_spam, char *secret);
void store_free(store *st);

