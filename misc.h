/*
 * misc.h, 2007.12.16, SJ
 */

#include <sys/time.h>
#include "config.h"

void _fatal(char *s);
void __fatal(char *s);
long tvdiff(struct timeval a, struct timeval b);
void pre_translate(char *p);
int translate(unsigned char *p, int qp);
int count_invalid_junk(unsigned char *p);
int count_invalid_hexa_stuff(unsigned char *p);
int is_odd_punctuations(char *p);
int is_number(char *p);
int is_hex_number(char *p);
int is_month(char *s);
int search_in_buf(char *s, int len1, char *what, int len2);
int count_char_in_buffer(char *p, char c);
void degenerate(unsigned char *p);
void fix_url(char *url);
char *str_case_str(char *s, char *what);
char *split(char *row, int ch, char *s, int size);
char *split_str(char *row, char *what, char *s, int size);
unsigned long long APHash(char *p);
void trim(char *what);
void replace(char *p, int what, int with);
int extract_email(char *rawmail, char *email);
int readfrompool(int fd, void *_s, size_t n);
int get_random_bytes(unsigned char *buf, int len);
int recvtimeout(int s, char *buf, int len, int timeout);
int make_rnd_string(char *res);
int is_valid_id(char *p);
void log_ham_spam_per_email(char *tmpfile, char *email, int ham_or_spam);
float calc_spamicity(float NHAM, float NSPAM, unsigned int nham, unsigned int nspam, float rob_s, float rob_x, int freq_min);
int qcache_socket(char *qcache_addr, int qcache_port, char *qcache_socket);
void write_delivery_info(char *tmpfile, char *dir, char *mailfrom, char rcptto[MAX_RCPT_TO][MAXBUFSIZE], int num_of_rcpt_to);
int move_message_to_quarantine(char *tmpfile, char *quarantine_dir, char *mailfrom, char rcptto[MAX_RCPT_TO][MAXBUFSIZE], int num_of_rcpt_to);

double check_bayes(char *file);

char trainbuf[SMALLBUFSIZE];
