#ifndef _PARSER_H
 #define _PARSER_H

#include <kospam.h>

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

#define SMALLBUFSIZE 512
#define MAXBUFSIZE 8192
#define BIGBUFSIZE 131072

// Structure to store headers
typedef struct {
    char *received;
    char *from;
    char *subject;
    char *message_id;
    char *content_type;
    char *content_encoding;
    char *kospam_envelope_from;
    char *kospam_envelope_recipient;
    char *kospam_xforward;
} Email;

extern Email *email;
extern int has_image_attachment;
extern int has_octet_stream_attachment;
extern int n_attachments;
extern struct attachment attachments[MAX_ATTACHMENTS];

struct Body {
    char data[BIGBUFSIZE];
    size_t pos;
};

int parse_message(const char *message, struct __state *state, struct Body *BODY);
int post_parse(struct __state *state, struct Body *BODY, struct __config *cfg);
char *read_file(const char *filename, size_t *outsize);
int parse_eml_buffer(char *buffer, struct Body *b);
void extract_mime_parts(char *body, const char *boundary, struct Body *b);

int base64_decode(char *input);
void decodeQP(char *p);
char *decode_mime_encoded_words(const char *input);
char *extract_header_value(const char *buffer, int buffer_len, const char *header_name, int header_name_len);
void extract_name_from_header_line(char *buffer, char *name, char *resultbuf, int resultbuflen);
char *find_boundary(const char *buffer);
void fixup_encoded_header(char *buf, int buflen);
void free_email(Email *email);
char *split(char *str, int ch, char *buf, int buflen, int *result);
void normalize_buffer(char *s);
int utf8_encode(char *inbuf, int inbuflen, char *outbuf, int outbuflen, char *encoding);
void utf8_tolower(char *s);
void decode_html_entities_utf8_inplace(char *buffer);
void normalize_html(char *input);
void chop_newlines(char *str, size_t len);

#endif /* _PARSER_H */
