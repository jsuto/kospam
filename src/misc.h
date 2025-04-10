/*
 * misc.h, SJ
 */

#ifndef _MISC_H
 #define _MISC_H

#include <sys/socket.h>
#include <openssl/ssl.h>
#include <sys/time.h>
#include <pwd.h>
#include <cfg.h>
#include "defs.h"

#define CHK_NULL(x, errmsg) if ((x)==NULL) { printf("error: %s\n", errmsg); return ERR; }
#define CHK_SSL(err, msg) if ((err)==-1) { printf("ssl error: %s\n", msg); return ERR; }

void __fatal(char *s);
long tvdiff(struct timeval a, struct timeval b);
int search_string_in_buffer(char *s, int len1, char *what, int len2);
int count_character_in_buffer(char *p, char c);
void replace_character_in_buffer(char *p, char from, char to);
char *split_str(char *row, char *what, char *s, int size);
int trim_buffer(char *s);
int extract_verp_address(char *email);
int extractEmail(char *rawmail, char *email);
void make_random_string(char *buf, int buflen);
int is_valid_clapf_id(char *p);
int recvtimeout(int s, char *buf, int len, int timeout);
int write1(int sd, void *buf, int buflen, int use_ssl, SSL *ssl);
int recvtimeoutssl(int s, char *buf, int len, int timeout, int use_ssl, SSL *ssl);

void write_pid_file(char *pidfile);
int drop_privileges(struct passwd *pwd);

void init_session_data(struct session_data *sdata);
int read_from_stdin(struct session_data *sdata);
void strtolower(char *s);

void *get_in_addr(struct sockaddr *sa);

int can_i_write_current_directory();

int is_item_on_list(char *item, char *list, char *extralist);
int is_list_on_string(char *s, char *list);
int is_dotted_ipv4_address(char *s);

long get_local_timezone_offset();

#endif /* _MISC_H */
