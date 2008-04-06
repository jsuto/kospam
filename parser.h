/*
 * parser.h, 2008.04.05, SJ
 */

#ifndef _PARSER_H
 #define _PARSER_H

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

struct _token {
   char str[MAX_TOKEN_LEN];
   unsigned long num;
   struct _token *r;
};

struct attachment {
   int size;
   char type[SMALLBUFSIZE];
};

struct _state {
   int message_state;
   int is_header;
   int has_boundary;
   int has_boundary2;
   int cnt_type;
   int textplain;
   int base64;
   int utf8;
   int iso_8859_2;
   int qp;
   int html_comment;
   int base64_text;
   int ipcnt;
   int check_attachment;
   int has_to_dump;
   int fd;
   int num_of_msword;
   int num_of_images;
   int unknown_client;
   int train_mode;
   unsigned long c_shit;
   unsigned long c_hex_shit;
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
   struct _token *c_token;
   struct _token *first;

   int n_attachments;
   struct attachment attachments[MAX_ATTACHMENTS];
};

struct session_data {
   char ttmpfile[3*RND_STR_LEN+1], mailfrom[MAXBUFSIZE], rcptto[MAX_RCPT_TO][MAXBUFSIZE], client_addr[IPLEN], name[SMALLBUFSIZE];
   unsigned long uid;
   int tot_len, num_of_rcpt_to, skip_id_check;
};


struct c_res {
   double spaminess;
   float ham_msg, spam_msg;
};

struct _state init_state();
int attachment_by_type(struct _state state, char *type);
int extract_boundary(char *p, char *boundary, int boundary_len);
struct _state parse(char *buf, struct _state st);
struct _state insert_token(struct _state state, char *p);
struct _token *new_token(char *s);
void free_and_print_list(struct _token *t, int print);

#endif /* _PARSER_H */
