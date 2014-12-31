/*
 * decoder.h, SJ
 */

#ifndef _DECODER_H
 #define _DECODER_H

void base64_encode(unsigned char *in, int inlen, char *out, int outlen);

void sanitiseBase64(char *s);
int decodeBase64(char *p);
int decode_base64_to_buffer(char *p, int plen, unsigned char *b, int blen);
void decodeQP(char *p);
void decodeHTML(char *p, int utf8);
void decodeURL(char *p);
inline void utf8_encode_char(unsigned char c, unsigned char *buf, int buflen, int *len);
int utf8_encode(char *inbuf, int inbuflen, char *outbuf, int outbuflen, char *encoding);

#endif /* _DECODER_H */
