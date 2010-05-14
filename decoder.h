/*
 * decoder.h, 2010.05.10, SJ
 */

#ifndef _DECODER_H
 #define _DECODER_H

void sanitiseBase64(char *s);
int decodeBase64(char *p, char *r);
void decodeUTF8(char *p);
void decodeQP(char *p);
void decodeHTML(char *p);
void decodeURL(char *p);

#endif /* _DECODER_H */
