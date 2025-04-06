#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <parser.h>
#include <config.h>


int utf8_encode(char *inbuf, int inbuflen, char *outbuf, int outbuflen, char *encoding){
   iconv_t cd;
   size_t inbytesleft, outbytesleft;
   int ret = ERR;

   memset(outbuf, 0, outbuflen);

   if(strcasecmp(encoding, "gb2312") == 0)
      cd = iconv_open("utf-8", "cp936");
   else if(strcasecmp(encoding, "ks_c_5601-1987") == 0)
      cd = iconv_open("utf-8", "EUC-KR");
   else
      cd = iconv_open("utf-8", encoding);

   if(cd != (iconv_t)-1){
      inbytesleft = inbuflen;
      outbytesleft = outbuflen-1;

      if(iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t) -1)
         ret = ERR;
      else
         ret = OK;

      iconv_close(cd);
   }

   return ret;
}


// Lowercase UTF-8 in-place (ASCII + Latin-1 via inline table)
void utf8_tolower(char *s) {
    unsigned char *p = (unsigned char *)s;

    while (*p) {
        if (*p < 0x80) {
            // ASCII A–Z → a–z
            if (*p >= 'A' && *p <= 'Z') {
                *p += 32;
            }
            p++;
        } else if (p[0] == 0xC3 && p[1] >= 0x80 && p[1] <= 0xBF) {
            // Latin-1 Supplement (U+00C0–U+00FF)
            static const unsigned char latin1_lc_map[64] = {
                // Map C3 80–9E (uppercases) → lowercase equivalents
                0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
                0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
                0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
                0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
                // C3 A0–BF already lowercase or unchanged
                0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
                0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
                0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
                0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF
            };
            p[1] = latin1_lc_map[p[1] - 0x80];
            p += 2;
        } else if ((p[0] & 0xE0) == 0xC0) {
            // Other 2-byte UTF-8 (non-Latin1) — skip
            p += 2;
        } else if ((p[0] & 0xF0) == 0xE0) {
            p += 3;
        } else if ((p[0] & 0xF8) == 0xF0) {
            p += 4;
        } else {
            // Invalid byte
            p++;
        }
    }
}
