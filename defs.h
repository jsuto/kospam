/*
 * defs.h, 2009.01.09, SJ
 */

#ifndef _DEFS_H
   #define _DEFS_H

#ifdef HAVE_MYSQL
  #include <mysql.h>
#endif
#ifdef HAVE_SQLITE3
  #include <sqlite3.h>
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

struct _state {
   int message_state;
   int is_header;
   int has_boundary;
   int has_boundary2;
   int cnt_type;
   int textplain;
   int texthtml;
   int base64;
   int utf8;
   int iso_8859_2;
   int qp;
   int html_comment;
   int base64_text;
   int base64_lines;
   int ipcnt;
   int check_attachment;
   int has_to_dump;
   int fd;
   int num_of_msword;
   int num_of_images;
   int train_mode;
   unsigned long c_shit;
   unsigned long l_shit;
   unsigned long line_num;
   char ctype[MAXBUFSIZE];
   char boundary[BOUNDARY_LEN];
   char boundary2[BOUNDARY_LEN];
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

   int n_attachments;
   struct attachment attachments[MAX_ATTACHMENTS];

   struct node *token_hash[MAXHASH];
};

struct session_data {
   char ttmpfile[SMALLBUFSIZE], deliveryinfo[SMALLBUFSIZE];
   char mailfrom[MAXBUFSIZE], rcptto[MAX_RCPT_TO][MAXBUFSIZE], client_addr[IPLEN], name[SMALLBUFSIZE];
   unsigned long uid;
   int fd, tot_len, num_of_rcpt_to, skip_id_check, need_signo_check, unknown_client;
   float Nham, Nspam;
#ifdef HAVE_MYSQL
   MYSQL mysql;
#endif
#ifdef HAVE_SQLITE3
   sqlite3 *db;
#endif
#ifdef HAVE_MYDB
   struct mydb_node *mhash[MAX_MYDB_HASH];
#endif
};


struct te {
   unsigned int nham;
   unsigned int nspam;
};


struct ue {
   unsigned long uid;
   unsigned int policy_group;
   char name[SMALLBUFSIZE];
};


#endif /* _DEFS_H */

