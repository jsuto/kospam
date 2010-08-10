/*
 * misc.h, SJ
 */

#ifndef _MISC_H
 #define _MISC_H

#define FLAG_IGNORE_WHITESPACE 1
#define FLAG_IGNORE_HEADERS 2

#include <sys/time.h>
#include <cfg.h>
#include "defs.h"

void __fatal(char *s);
long tvdiff(struct timeval a, struct timeval b);
int searchStringInBuffer(char *s, int len1, char *what, int len2);
int countCharacterInBuffer(char *p, char c);
void replaceCharacterInBuffer(char *p, char from, char to);
char *split(char *row, int ch, char *s, int size);
char *split_str(char *row, char *what, char *s, int size);
unsigned long long APHash(char *p);
void trimBuffer(char *s);
int extractEmail(char *rawmail, char *email);
void createClapfID(char *id);
int getRandomBytes(unsigned char *buf, int len);
int readFromEntropyPool(int fd, void *_s, size_t n);
int recvtimeout(int s, char *buf, int len, int timeout);
int isValidClapfID(char *p);
int extract_id_from_message(char *messagefile, char *clapf_header_field, char *ID);
void writeDeliveryInfo(struct session_data *sdata, char *dir);

unsigned int spamsum_match_db(const char *fname, const char *sum, unsigned int threshold);
char *spamsum_file(const char *fname, unsigned int flags, unsigned int block_size);

int isDottedIPv4Address(char *s);
int isEmailAddressOnList(char *list, char *tmpfile, char *email, struct __config *cfg);

#ifndef _GNU_SOURCE
   char *strcasestr(const char *s, const char *find);
#endif

#endif /* _MISC_H */
