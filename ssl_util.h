/*
 * ssl_util.h, 2008.03.04, SJ
 */

int read1(int s, char *buf, int use_ssl, SSL *ssl);
int write1(int sd, char *buf, int use_ssl, SSL *ssl);

