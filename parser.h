/*
 * parser.h, 2007.06.18, SJ
 */

#include "config.h"

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
   int is_header;
   int is_subject;
   int is_from;
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
   unsigned long c_shit;
   unsigned long c_hex_shit;
   unsigned long l_shit;
   unsigned long line_num;
   char ctype[MAXBUFSIZE];
   char boundary[BOUNDARY_LEN];
   char boundary2[BOUNDARY_LEN];
   char ip[IPLEN];
   char miscbuf[MAX_TOKEN_LEN];
   char qpbuf[MAX_TOKEN_LEN];
   char attachedfile[RND_STR_LEN+SMALLBUFSIZE];
   char from[SMALLBUFSIZE];
   unsigned long n_token;
   unsigned long n_body_token;
   unsigned long n_chain_token;
   struct _token *c_token;
   struct _token *first;

   int n_attachments;
   struct attachment attachments[MAX_ATTACHMENTS];
};

struct session_data {
   char ttmpfile[RND_STR_LEN+1], mailfrom[MAXBUFSIZE], rcptto[MAX_RCPT_TO][MAXBUFSIZE];
   unsigned long uid;
   int tot_len, num_of_rcpt_to;
};

struct _state init_state();
int attachment_by_type(struct _state state, char *type);
int extract_boundary(char *p, char *boundary, int boundary_len);
struct _state parse(char *buf, struct _state st);
struct _state insert_token(struct _state state, char *p);
struct _token *new_token(char *s);
void free_and_print_list(struct _token *t, int print);
