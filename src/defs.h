/*
 * defs.h, SJ
 */

#ifndef _DEFS_H
   #define _DEFS_H

#ifdef NEED_MYSQL
  #include <mysql.h>
  #include <mysqld_error.h>
#endif
#ifdef HAVE_TRE
   #include <tre/tre.h>
   #include <tre/regex.h>
#endif
#include <tcpd.h>

#include <openssl/sha.h>
#include <openssl/ssl.h>
#include "tai.h"
#include "config.h"

#define MSG_UNDEF -1
#define MSG_BODY 0
#define MSG_RECEIVED 1
#define MSG_FROM 2
#define MSG_TO 3
#define MSG_CC 4
#define MSG_SUBJECT 5
#define MSG_CONTENT_TYPE 6
#define MSG_CONTENT_TRANSFER_ENCODING 7
#define MSG_CONTENT_DISPOSITION 8
#define MSG_MESSAGE_ID 9
#define MSG_REFERENCES 10
#define MSG_RECIPIENT 11

#define DELIM ''

#define PARTITION_MGMT_INTERVAL 1800

#define MAXHASH 8171

#define NUM_OF_REGEXES 30

#define BASE64_RATIO 1.33333333

#define DIGEST_LENGTH SHA256_DIGEST_LENGTH

#define REAL_HAM_TOKEN_PROBABILITY  0.0001
#define REAL_SPAM_TOKEN_PROBABILITY 0.9999

#define MAX_ITERATIVE_TRAIN_LOOPS 5

#define UNDEF 0
#define READY 1
#define BUSY 2

#define MAX_SQL_VARS 20

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

#define GROUP_SHARED 0
#define GROUP_MERGED 1

#define T_TOE 0
#define T_TUM 1

typedef void signal_func (int);
typedef unsigned long long uint64;

struct child {
   pid_t pid;
   int messages;
   int status;
   int serial;
};


struct attachment {
   int size;
   char type[TINYBUFSIZE];
   char shorttype[TINYBUFSIZE];
   char aname[TINYBUFSIZE];
   char filename[TINYBUFSIZE];
   char internalname[TINYBUFSIZE];
   char digest[2*DIGEST_LENGTH+1];
   char dumped;
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


struct __state {
   int line_num;
   int message_state;
   int is_header;
   int is_1st_header;
   int textplain;
   int texthtml;
   int message_rfc822;
   int base64;
   int utf8;
   int qp;
   int htmltag;
   int style;
   int skip_html;
   int has_to_dump;
   int attachment;
   int fd;
   int b64fd;
   int pushed_pointer;
   int abufpos;
   int c_shit;
   int l_shit;
   char attachedfile[RND_STR_LEN+SMALLBUFSIZE];
   char message_id[SMALLBUFSIZE];
   char miscbuf[MAX_TOKEN_LEN];
   char qpbuf[MAX_TOKEN_LEN];
   int n_token;
   int n_subject_token;
   int n_deviating_token;

   char filename[TINYBUFSIZE];
   char type[TINYBUFSIZE];
   char charset[TINYBUFSIZE];

   char attachment_name_buf[SMALLBUFSIZE];
   int anamepos;

   struct node *boundaries[MAXHASH];

   int n_attachments;
   struct attachment attachments[MAX_ATTACHMENTS];

   char from[SMALLBUFSIZE];

   char b_from[SMALLBUFSIZE], b_from_domain[SMALLBUFSIZE], b_subject[MAXBUFSIZE], b_body[BIGBUFSIZE];

   struct node *token_hash[MAXHASH];
   struct node *url[MAXHASH];

   int found_our_signo;
   int train_mode;

   unsigned int bodylen;
};


struct session_data {
   char filename[SMALLBUFSIZE];
   char ttmpfile[SMALLBUFSIZE];
   char mailfrom[SMALLBUFSIZE], rcptto[MAX_RCPT_TO][SMALLBUFSIZE];
   char clapf_id[SMALLBUFSIZE];
   char fromemail[SMALLBUFSIZE];
   char acceptbuf[SMALLBUFSIZE];
   char attachments[SMALLBUFSIZE];
   char ip[SMALLBUFSIZE];
   char hostname[SMALLBUFSIZE];
   char whitelist[MAXBUFSIZE], blacklist[MAXBUFSIZE];
   char name[MAXBUFSIZE], domain[MAXBUFSIZE];
   char spaminessbuf[MAXBUFSIZE];
   char tre;
   unsigned int status;
   int trapped_client;
   int from_address_in_mydomain;
   int tls;
   int ipcnt;
   int fd, hdr_len, tot_len, stored_len, num_of_rcpt_to, rav;
   int statistically_whitelisted;
   int need_scan, need_signo_check;
   int policy_group, blackhole;
   int training_request;
   int mynetwork;
   unsigned int uid, gid;
   float __acquire, __parsed, __av, __user, __policy, __training, __update, __store, __inject, __as, __minefield;
   float spaminess;
   float nham, nspam;
   char bodydigest[2*DIGEST_LENGTH+1];
   char digest[2*DIGEST_LENGTH+1];
   time_t now, sent;
   pid_t pid;
   unsigned int sql_errno;
#ifdef NEED_MYSQL
   MYSQL mysql;
#endif
};


#ifdef HAVE_MEMCACHED

#include <stdbool.h>
#include <netinet/in.h>

struct flags {
   bool no_block:1;
   bool no_reply:1;
   bool tcp_nodelay:1;
   bool tcp_keepalive:1;
};


struct memcached_server {

   struct flags flags;

   int fd;
   unsigned int snd_timeout;
   unsigned int rcv_timeout;

   int send_size;
   int recv_size;
   unsigned int tcp_keepidle;

   int last_read_bytes;

   char *result;
   char buf[MAXBUFSIZE];

   struct sockaddr_in addr;

   char server_ip[IPLEN];
   int server_port;

   char initialised;
};
#endif


struct __data {
   int quiet;
#ifdef HAVE_TRE
   int n_regex;
   regex_t pregs[NUM_OF_REGEXES];
#endif
   char starttls[TINYBUFSIZE];
   struct node *mydomains[MAXHASH];

#ifdef HAVE_MEMCACHED
   struct memcached_server memc;
#endif

   SSL_CTX *ctx;
   SSL *ssl;
};


struct sql {
#ifdef NEED_MYSQL
   MYSQL_STMT *stmt;
#endif
   char *sql[MAX_SQL_VARS];
   int type[MAX_SQL_VARS];
   int len[MAX_SQL_VARS];
   unsigned long length[MAX_SQL_VARS];
   my_bool is_null[MAX_SQL_VARS];
   my_bool error[MAX_SQL_VARS];
   int pos;
};


struct __counters {
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

