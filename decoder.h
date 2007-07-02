/*
 * decoder.h, 2007.06.07, SJ
 */

void url_decode(char *p);
void sanitiseBase64(char *s);
int base64_decode(char *p, char *r);
void utf8_decode(unsigned char *p);
void qp_decode(unsigned char *p);
void html_decode(char *p);

