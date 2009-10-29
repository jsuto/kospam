/*
 * defs.h, 2009.10.29, SJ
 */

#ifndef _DEFS_H
   #define _DEFS_H

#ifdef NEED_MYSQL
  #include <mysql.h>
#endif
#ifdef NEED_SQLITE3
  #include <sqlite3.h>
#endif
#ifdef NEED_LDAP
  #include <ldap.h>
#endif
#ifdef HAVE_MEMCACHED
  #include <libmemcached/memcached.h>
#endif
#ifdef HAVE_LIBCLAMAV
   #include <clamav.h>
#endif
#ifdef HAVE_TRE
   #include <tre/tre.h>
   #include <tre/regex.h>
#endif

#include "config.h"

#define MSG_UNDEF -1
#define MSG_BODY 0
#define MSG_RECEIVED 1
#define MSG_FROM 2
#define MSG_TO 3
#define MSG_SUBJECT 4
#define MSG_CONTENT_TYPE 5
#define MSG_CONTENT_TRANSFER_ENCODING 6
#define MSG_CONTENT_DISPOSITION 7


#define MYDB_HEADER_SIZE 8
#define N_SIZE 16
#define SEGMENT_SIZE 768
#define MAX_MYDB_HASH 74713
#define MAX_TOKENS_TO_UPDATE_IN_1_ROUND 65535

#define MAXHASH 8171


struct node {
   char str[MAX_TOKEN_LEN];
   unsigned long long key;
   double spaminess;
   double deviation;
   float nham;
   float nspam;
   unsigned long num;
   char type;
   struct node *r;
};

struct mydb {
   unsigned long long key;
   unsigned short int nham;
   unsigned short int nspam;
   unsigned long ts;
};

struct mydb_node {
   unsigned long long key;
   unsigned short int nham;
   unsigned short int nspam;
   unsigned long ts;
   unsigned int pos;
   struct mydb_node *r;
};

struct x_token {
   unsigned long long token;
   float spaminess;
};

struct attachment {
   int size;
   char type[SMALLBUFSIZE];
};

struct url {
   char url_str[URL_LEN];
   struct url *r;
};

struct boundary {
   char boundary_str[BOUNDARY_LEN];
   struct boundary *r;
};

struct _state {
   int message_state;
   int is_header;
   int cnt_type;
   int textplain;
   int texthtml;
   int base64;
   int has_base64;
   int utf8;
   int iso_8859_2;
   int qp;
   int html_comment;
   int base64_text;
   int base64_lines;
   int ipcnt;
   int has_to_dump;
   int fd;
   int num_of_msword;
   int num_of_images;
   int train_mode;
   unsigned long c_shit;
   unsigned long l_shit;
   unsigned long line_num;
   char ctype[MAXBUFSIZE];
   char ip[SMALLBUFSIZE];
   char miscbuf[MAX_TOKEN_LEN];
   char qpbuf[MAX_TOKEN_LEN];
   char attachedfile[RND_STR_LEN+SMALLBUFSIZE];
   char from[SMALLBUFSIZE];
   unsigned long n_token;
   unsigned long n_subject_token;
   unsigned long n_body_token;
   unsigned long n_chain_token;
   struct url *urls;

   int found_our_signo;

   struct boundary *boundaries;

   int n_attachments;
   struct attachment attachments[MAX_ATTACHMENTS];

   struct node *token_hash[MAXHASH];
};

struct session_data {
   char ttmpfile[SMALLBUFSIZE], deliveryinfo[SMALLBUFSIZE], clapf_id[SMALLBUFSIZE], xforward[SMALLBUFSIZE], tre;
   char mailfrom[SMALLBUFSIZE], rcptto[MAX_RCPT_TO][SMALLBUFSIZE], client_addr[IPLEN], name[SMALLBUFSIZE], domain[SMALLBUFSIZE];
   char spaminessbuf[MAXBUFSIZE], acceptbuf[SMALLBUFSIZE];
   unsigned long uid;
   int fd, tot_len, num_of_rcpt_to, skip_id_check, need_signo_check, unknown_client, trapped_client, rav;
   int policy_group, blackhole;
   int need_scan;
   int training_request;
   float spaminess;
   float Nham, Nspam;
   float __acquire, __parsed, __av, __user, __policy, __training, __minefield, __as, __update, __store, __inject;
#ifdef HAVE_MAILBUF
   char mailbuf[MAILBUFSIZE], discard_mailbuf;
   int message_size, mailpos;
#endif
#ifdef NEED_MYSQL
   MYSQL mysql;
#endif
#ifdef NEED_SQLITE3
   sqlite3 *db;
#endif
#ifdef NEED_LDAP
   LDAP *ldap;
#endif
#ifdef HAVE_MYDB
   struct mydb_node *mhash[MAX_MYDB_HASH];
#endif
#ifdef HAVE_MEMCACHED
   memcached_st *memc;
#endif
};


struct te {
   unsigned int nham;
   unsigned int nspam;
};


struct __data {
   struct url *blackhole;

#ifdef HAVE_LIBCLAMAV
   struct cl_engine *engine;
#endif

#ifdef HAVE_TRE
   regex_t pregs[NUM_OF_REGEXES];
   int n_regex;
#endif

};


#endif /* _DEFS_H */

