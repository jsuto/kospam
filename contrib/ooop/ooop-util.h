/*
 * ooop-util.h, 2008.06.03, SJ
 */

#ifndef _OOOP_UTIL_H
 #define _OOOP_UTIL_H

#include <openssl/ssl.h>

int recvtimeoutssl(int s, char *buf, int len, int timeout, int use_ssl, SSL *ssl);
int write1(int sd, char *buf, int use_ssl, SSL *ssl);
void drop_root(int uid, int gid);
unsigned int djb_hash(char *str);
void get_path_by_name(char *s, char **path);

#endif /* _OOOP_UTIL_H */

