#ifndef _OOOP_UTIL_H
 #define _OOOP_UTIL_H

#include <openssl/ssl.h>

int recvtimeoutssl(int s, char *buf, int len, int timeout, int use_ssl, SSL *ssl);
int write1(int sd, char *buf, int use_ssl, SSL *ssl);
void drop_root(int uid, int gid);

#endif /* _OOOP_UTIL_H */

