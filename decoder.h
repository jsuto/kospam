/*
 * decoder.h, 2008.04.05, SJ
 */

#ifndef _DECODER_H
 #define _DECODER_H

void url_decode(char *p);
void sanitiseBase64(char *s);
int base64_decode(char *p, char *r);
void utf8_decode(unsigned char *p);
void qp_decode(unsigned char *p);
void html_decode(char *p);

#endif /* _DECODER_H */
