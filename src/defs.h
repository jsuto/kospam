/*
 * defs.h, SJ
 */

#ifndef _DEFS_H
   #define _DEFS_H

#ifdef HAVE_TRE
   #include <tre/tre.h>
   #include <tre/regex.h>
#endif

#include <openssl/sha.h>
#include <openssl/ssl.h>
#include "config.h"

#define PARTITION_MGMT_INTERVAL 1800

#define MAXHASH 8171

#define NUM_OF_REGEXES 30

#define DIGEST_LENGTH SHA256_DIGEST_LENGTH

#define REAL_HAM_TOKEN_PROBABILITY  0.0001
#define REAL_SPAM_TOKEN_PROBABILITY 0.9999

#define MAX_ITERATIVE_TRAIN_LOOPS 5

#define UNDEF 0
#define READY 1
#define BUSY 2

#define TYPE_UNDEF 0
#define TYPE_SHORT 1
#define TYPE_LONG 2
#define TYPE_LONGLONG 3
#define TYPE_STRING 4

#define MAXCHILDREN 64

#define DEFAULT_SPAMICITY 0.5
#define MAX_KEY_VAL 18446744073709551615ULL

#define JUNK_REPLACEMENT_CHAR 'X'

#define NUMBER_OF_GOOD_FROM 10

typedef unsigned long long uint64;

struct child {
   pid_t pid;
   int messages;
   int status;
   int serial;
};


typedef struct {
   double mant;
   int exp;
} FLOAT;


struct te {
   unsigned int nham;
   unsigned int nspam;
};


struct node {
   void *str;
   uint64 key;
   double spaminess;
   double deviation;
   float nham;
   float nspam;
   char type;
   struct node *r;
};


struct rule {
#ifdef HAVE_TRE
   regex_t from;
   regex_t to;
   regex_t subject;
   regex_t attachment_name;
   regex_t attachment_type;
#endif
   int spam;
   int size;
   char _size[4];
   int attachment_size;
   char _attachment_size[4];

   char *domain;
   int domainlen;

   int days;

   char emptyfrom, emptyto, emptysubject, emptyaname, emptyatype;

   char *rulestr;
   char compiled;

   struct rule *r;
};


struct session_data {
   char ttmpfile[SMALLBUFSIZE];
   char whitelist[MAXBUFSIZE], blacklist[MAXBUFSIZE];
   char spaminessbuf[MAXBUFSIZE];
   unsigned int status;
   int trapped_client;
   int tot_len;
   int statistically_whitelisted;
   int blackhole;
   int mynetwork;
   float __parsed, __user, __training, __update, __as, __minefield;
   float spaminess;
   float nham, nspam;
};


struct data {
   int quiet;
#ifdef HAVE_TRE
   int n_regex;
   regex_t pregs[NUM_OF_REGEXES];
#endif
};


struct counters {
   uint64 c_rcvd;
   uint64 c_ham;
   uint64 c_spam;
   uint64 c_possible_spam;
   uint64 c_unsure;
   uint64 c_minefield;
   uint64 c_virus;
   uint64 c_zombie;
   uint64 c_fp;
   uint64 c_fn;
   uint64 c_mynetwork;
   uint64 c_size;
};

#endif /* _DEFS_H */
