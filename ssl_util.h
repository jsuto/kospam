/*
 * ssl_util.h, 2007.11.08, SJ
 */

#include <openssl/ssl.h>

int read1(int sd, char *buf, int use_ssl, SSL *ssl);
int write1(int sd, char *buf, int use_ssl, SSL *ssl);

