/*
 * misc.h, 2009.09.02, SJ
 */

#ifndef _MISC_H
 #define _MISC_H

#define FLAG_IGNORE_WHITESPACE 1
#define FLAG_IGNORE_HEADERS 2

#include <sys/time.h>
#include <cfg.h>
#include "defs.h"
#include <config.h>

void _fatal(char *s);
void __fatal(char *s);
long tvdiff(struct timeval a, struct timeval b);
void pre_translate(char *p);
int translate(unsigned char *p, int qp);
int translate2(unsigned char *p, int qp);
void reassemble_token(char *p);
int count_invalid_junk(char *p, int replace_junk);
int is_odd_punctuations(char *p);
int is_number(char *p);
int is_hex_number(char *p);
int is_month(char *s);
int search_in_buf(char *s, int len1, char *what, int len2);
int count_char_in_buffer(char *p, char c);
void degenerate(unsigned char *p);
void fix_url(char *url);
void tld_from_url(char *url);
void fix_fqdn(char *fqdn);
void tld_from_fqdn(char *fqdn);
char *split(char *row, int ch, char *s, int size);
char *split_str(char *row, char *what, char *s, int size);
unsigned long long APHash(char *p);
void trim(char *what);
void replace(char *p, int what, int with);
int extract_email(char *rawmail, char *email);
int readfrompool(int fd, void *_s, size_t n);
int get_random_bytes(unsigned char *buf, int len);
int recvtimeout(int s, char *buf, int len, int timeout);
int recvtimeout2(int s, unsigned char *buf, int len, int timeout);
int make_rnd_string(char *res);
int is_valid_id(char *p);
int extract_id_from_message(char *messagefile, char *clapf_header_field, char *ID);
void write_delivery_info(struct session_data *sdata, char *dir);
int move_message_to_quarantine(struct session_data *sdata, char *quarantine_dir);
int is_recipient_in_our_domains(char *rawmail,  struct __config *cfg);

int rbl_check(char *rbldomain, char *host, int debug);
int reverse_ipv4_addr(char *ip);
int rbl_list_check(char *domainlist, char *hostlist, int debug);

unsigned long blackness(char *dir, char *ip, struct __config *cfg);
void put_ip_to_dir(char *dir, char *ip);

double gsl_chi2inv(double x, double df);
double chi2inv(double x, double df, double esf);

unsigned int spamsum_match_db(const char *fname, const char *sum, unsigned int threshold);
char *spamsum_file(const char *fname, unsigned int flags, unsigned int block_size);

unsigned long resolve_host(char *h);

int whitelist_check(char *whitelist, char *tmpfile, char *email, struct __config *cfg);

#endif /* _MISC_H */
