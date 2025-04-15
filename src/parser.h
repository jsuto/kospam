#ifndef _PARSER_H
 #define _PARSER_H

#include <stdbool.h>
#include <config.h>
#include <defs.h>
#include <cfg.h>

#define HEADER_FROM "From:"
#define HEADER_SUBJECT "Subject:"
#define HEADER_MESSAGE_ID "Message-ID:"
#define HEADER_CONTENT_TYPE "Content-Type:"
#define HEADER_CONTENT_TRANSFER_ENCODING "Content-Transfer-Encoding:"
#define HEADER_CONTENT_DISPOSITION "Content-Disposition:"
#define HEADER_RECEIVED "Received:"

// Kospam-* headers
#define HEADER_KOSPAM_ENVELOPE_FROM "Kospam-Envelope-From: "
#define HEADER_KOSPAM_ENVELOPE_RECIPIENT "Kospam-Envelope-Recipient: "
#define HEADER_KOSPAM_XFORWARD "Kospam-Xforward: "

#define HEADER_KOSPAM_WATERMARK "X-Kospam-Watermark: "

#define SMALLBUFSIZE 512
#define MAXBUFSIZE 8192
#define BIGBUFSIZE 131072

struct Header {
    char received[MAXBUFSIZE];
    char from[SMALLBUFSIZE];
    char subject[2*SMALLBUFSIZE];
    char message_id[2*SMALLBUFSIZE];
    char content_type[SMALLBUFSIZE];
    char content_encoding[SMALLBUFSIZE];
    char kospam_watermark[SMALLBUFSIZE];
};

struct Kospam {
    char kospam_envelope_from[SMALLBUFSIZE];
    char kospam_envelope_recipient[MAXBUFSIZE];
    char kospam_xforward[2*SMALLBUFSIZE];
};

struct Body {
    char data[BIGBUFSIZE];
    size_t pos;
};

struct attachment {
   size_t size;
   char type[TINYBUFSIZE];
   char filename[TINYBUFSIZE];
   char digest[2*DIGEST_LENGTH+1];
};

struct Message {
    struct Kospam kospam;
    struct Header header;
    struct Body body;
    struct attachment attachments[MAX_ATTACHMENTS];
    int n_attachments;
    bool need_signo_check;
    bool found_our_signo;
    char has_image_attachment;
    char has_octet_stream_attachment;
};

struct parser_state {
   char tre;
   int n_token;
   int n_subject_token;
   int n_deviating_token;

   char from[SMALLBUFSIZE];
   char b_subject[MAXBUFSIZE];

   struct node *token_hash[MAXHASH];

   char envelope_from[SMALLBUFSIZE];
   char envelope_recipient[SMALLBUFSIZE];

   bool need_signo_check;
   bool found_our_signo;

   int training_request;

   char has_image_attachment;
   char has_octet_stream_attachment;

   char ip[SMALLBUFSIZE];
   char hostname[SMALLBUFSIZE];

   char kospam_watermark[SMALLBUFSIZE];

   bool trapped;
};

void init_state(struct parser_state *state);
int parse_message(const char *message, struct parser_state *state, struct Message *m, struct config *cfg);
int post_parse(struct parser_state *state, struct Message *m, struct config *cfg);
char *read_file(const char *filename, size_t *outsize);
int parse_eml_buffer(char *buffer, struct Message *m, struct config *cfg);
void extract_mime_parts(char *body, const char *boundary, struct Message *m, struct config *cfg);

int base64_decode(char *input);
void decodeQP(char *p);
char *decode_mime_encoded_words(const char *input);
void extract_header_value(const char *buffer, int buffer_len, const char *header_name, int header_name_len, char *result, size_t resultbuflen);
void extract_token_from_header_line(char *buffer, char *name, char *resultbuf, int resultbuflen);
void extract_name_from_headers(char *buf, char *resultbuf, size_t resultlen);
char *find_boundary(const char *buffer);
void fixup_encoded_header(char *buf, int buflen);
char *split(char *str, int ch, char *buf, int buflen, int *result);
void normalize_buffer(char *s);
int utf8_encode(char *inbuf, int inbuflen, char *outbuf, int outbuflen, char *encoding);
void utf8_tolower(char *s);
void decode_html_entities_utf8_inplace(char *buffer);
void normalize_html(char *input);
void chop_newlines(char *str, size_t len);
void extract_url_token(char *s, char *result, int resultlen);
bool is_item_on_list(char *item, char *list);
int count_character_in_buffer(char *p, char c);

#endif /* _PARSER_H */
