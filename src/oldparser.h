/*
 * oldparser.h, SJ
 */

#ifndef _OLDPARSER_H
 #define _OLDPARSER_H

#include "cfg.h"
#include "config.h"
#include "defs.h"

struct __state parse_message2(struct session_data *sdata, int take_into_pieces, struct __config *cfg);
void post_parse2(struct __state *state);
int parse_line(char *buf, struct __state *state, struct session_data *sdata, int take_into_pieces, char *abuffer, int abuffersize, struct __config *cfg);

void init_state(struct __state *state);
unsigned long parse_date_header(char *s, struct __config *cfg);
long get_local_timezone_offset();
int is_hex_number(char *p);
int extract_boundary(char *p, struct __state *state);
void fixupEncodedHeaderLine(char *buf, int buflen);
void fixupSoftBreakInQuotedPritableLine(char *buf, struct __state *state);
void fixupBase64EncodedLine(char *buf, struct __state *state);
void remove_html(char *buf, struct __state *state);
int append_html_tag(char *buf, char *htmlbuf, int pos, struct __state *state);
void translate_line(unsigned char *p, struct __state *state);
void fix_email_address_for_sphinx(char *s);
void split_email_address(char *s);
int does_it_seem_like_an_email_address(char *email);
void reassemble_token(char *p);
void degenerate_token(unsigned char *p);
void fix_URL(char *url);
void fix_FQDN(char *fqdn);
void get_tld_from_name(char *name);
int count_invalid_junk_characters(char *p, int replace_junk);
int count_invalid_junk_lines(char *p);
char *determine_attachment_type(char *filename, char *type);
char *get_attachment_extractor_by_filename(char *filename);
int base64_decode_attachment_buffer(char *p, unsigned char *b, int blen);

//int has_octet_stream(struct __state *state);
//int has_image_attachment(struct __state *state);

#endif /* _OLDPARSER_H */
