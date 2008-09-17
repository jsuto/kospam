/*
 * decoder.h, 2008.09.17, SJ
 */

#ifndef _DECODER_H
 #define _DECODER_H

void url_decode(char *p);
void sanitiseBase64(char *s);
int base64_decode(char *p, char *r);
void utf8_decode(char *p);
void qp_decode(char *p);
void html_decode(char *p);

#endif /* _DECODER_H */
