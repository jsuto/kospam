#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <parser.h>
#include <config.h>


void fixup_encoded_header(char *buf, int buflen){
   char *q, *r, *s, *e, *end;
   /*
    * I thought SMALLBUFSIZE would be enough for v, encoding and tmpbuf(2*),
    * but then I saw a 6-7000 byte long subject line, so I've switched to MAXBUFSIZE
    */
   char v[MAXBUFSIZE], u[MAXBUFSIZE], puf[MAXBUFSIZE], encoding[MAXBUFSIZE], tmpbuf[2*MAXBUFSIZE];
   int need_encoding, ret, prev_encoded=0, n_tokens=0;

   if(buflen < 5) return;

   memset(puf, 0, sizeof(puf));
   size_t puflen = 0;

   q = buf;
   int result;

   while(q) {
      q = split(q, ' ', v, sizeof(v), &result);

      char *p = v;

      do {
         memset(u, 0, sizeof(u));

         /*
          * We can't use split_str(p, "=?", ...) it will fail with the following pattern
          *    =?UTF-8?B?SG9neWFuIMOtcmp1bmsgcGFuYXN6bGV2ZWxldD8=?=
          *
          * Also the below pattern requires special care:
          *    =?gb2312?B?<something>?==?gb2312?Q?<something else>?=
          *
          * And we have to check the following cases as well:
          *    Happy New Year! =?utf-8?q?=F0=9F=8E=86?=
          */

         int b64=0, qp=0;

         memset(encoding, 0, sizeof(encoding));

         r = strstr(p, "=?");
         if(r){
            p = r + 2;

            e = strchr(p, '?');
            if(e){
               *e = '\0';
               snprintf(encoding, sizeof(encoding)-1, "%s", p);
               *e = '?';

               s = strcasestr(e, "?B?");
               if(s){
                  b64 = 1;
                  p = s + 3;
               }
               else {
                  s = strcasestr(e, "?Q?");
                  if(s){
                     qp = 1;
                     p = s + 3;
                  }
               }
            }

            end = strstr(p, "?=");
            if(end){
               *end = '\0';
            }


            snprintf(u, sizeof(u)-1, "%s", p);

            if(end) {
               p = end + 2;
            }
         }
         else {
            snprintf(u, sizeof(u)-1, "%s", p);
            p = NULL;
         }

         if(u[0] == 0) continue;

         n_tokens++;

         if(b64 == 1) base64_decode(u);
         else if(qp == 1) decodeQP(u);

         /*
          * https://www.ietf.org/rfc/rfc2047.txt says that
          *
          * "When displaying a particular header field that contains multiple
          *  'encoded-word's, any 'linear-white-space' that separates a pair of
          *  adjacent 'encoded-word's is ignored." (6.2)
          */

         if(prev_encoded == 1 && (b64 == 1 || qp == 1)) {}
         else if(n_tokens > 1){
            if (sizeof(puf) > puflen + 1) { memcpy(puf+puflen, " ", 1); puflen++; }
         }

         size_t copylen = 0;

         if(b64 == 1 || qp == 1){
            prev_encoded = 1;
            need_encoding = 0;
            ret = ERR;

            if(encoding[0] && strcasecmp(encoding, "utf-8")){
               need_encoding = 1;
               ret = utf8_encode(u, strlen(u), &tmpbuf[0], sizeof(tmpbuf), encoding);
            }

            if(need_encoding == 1 && ret == OK){
               copylen = strlen(tmpbuf);
               if (sizeof(puf) > puflen + copylen) { memcpy(puf+puflen, tmpbuf, copylen); puflen += copylen; }
            }
            else {
               copylen = strlen(u);
               if (sizeof(puf) > puflen + copylen) { memcpy(puf+puflen, u, copylen); puflen += copylen; }
            }
         }
         else {
            copylen = strlen(u);
            if (sizeof(puf) > puflen + copylen) { memcpy(puf+puflen, u, copylen); puflen += copylen; }
         }

      } while(p);

   }

   snprintf(buf, buflen, "%s", puf);
}


char *extract_header_value(const char *buffer, int buffer_len, const char *header_name, int header_name_len) {
    char *header_pos = NULL;
    const char *buffer_start = buffer;

    while ((header_pos = strcasestr(buffer_start, header_name)) != NULL) {
        // Check if this is at the beginning of a line
        if (header_pos == buffer ||
            *(header_pos-1) == '\n' ||
            (*(header_pos-1) == '\r' && *(header_pos-2) == '\n')) {

            // We found a valid header
            break;
        }

        // Move past this occurrence
        buffer_start = header_pos + header_name_len;
    }

    if (!header_pos) return NULL;

    // Skip header name and any whitespace
    const char *value_start = header_pos + header_name_len;
    while (*value_start && isspace(*value_start)) value_start++;

    // Find the end of this header (next header or end of headers)
    const char *value_end = value_start;
    const char *headers_end = buffer_start + buffer_len;

    while (value_end < headers_end) {
        // Look for the next header (line that doesn't start with whitespace)
        const char *next_line = strstr(value_end, "\r\n");
        if (!next_line) next_line = strstr(value_end, "\n");
        if (!next_line || next_line >= headers_end) {
            value_end = headers_end;
            break;
        }

        // Move past the newline
        next_line += (*(next_line) == '\r' ? 2 : 1);

        // If the next line starts with whitespace, it's a continuation
        if (*next_line == ' ' || *next_line == '\t') {
            value_end = next_line;
        } else {
            // Found the end of this header
            value_end = next_line - (*(next_line-2) == '\r' ? 2 : 1);
            break;
        }
    }

    // Calculate the value length
    size_t value_length = value_end - value_start;

    // Allocate memory and copy the value
    char *raw_value = (char *)malloc(value_length + 1);
    if (!raw_value) return NULL;

    // Copy and process the header value (unfold lines)
    size_t j = 0;
    bool prev_was_newline = false;

    for (size_t i = 0; i < value_length; i++) {
        char c = value_start[i];

        // Skip CR
        if (c == '\r') continue;

        // Convert sequence of CRLF + whitespace to a single space
        if (c == '\n') {
            prev_was_newline = true;
            continue;
        }

        if (prev_was_newline) {
            if (c == ' ' || c == '\t') {
                // Replace the folded line with a space, but only if we don't already have one
                if (j > 0 && raw_value[j-1] != ' ')
                    raw_value[j++] = ' ';
            } else {
                // Unexpected character after newline (should be whitespace for folded header)
                raw_value[j++] = ' ';
                raw_value[j++] = c;
            }
            prev_was_newline = false;
        } else {
            raw_value[j++] = c;
        }
    }

    raw_value[j] = '\0';

    if(!strcmp(header_name, HEADER_SUBJECT) ||
       !strcmp(header_name, HEADER_FROM)
      ) {
       fixup_encoded_header(raw_value, j+1);
    }

    return raw_value;
}


void extract_name_from_header_line(char *buffer, char *name, char *resultbuf, int resultbuflen) {
   char buf[SMALLBUFSIZE], *p, *q;

   snprintf(buf, sizeof(buf)-1, "%s", buffer);

   p = strstr(buf, "\n\r\n");
   if(p) {
      *p = '\0';
   } else {
      p = strstr(buf, "\n\n");
      if (p) *p = '\0';
   }

   memset(resultbuf, 0, resultbuflen);

   p = strstr(buf, name);
   if(p){
      int extended=0;

      /*
       *
       * Some examples from http://tools.ietf.org/html/rfc5987:
       *
       *   Non-extended notation, using "token":
       *
       *      foo: bar; title=Economy
       *
       *   Non-extended notation, using "quoted-string":
       *
       *      foo: bar; title="US-$ rates"
       *
       *   Extended notation, using the Unicode character U+00A3 (POUND SIGN):
       *
       *      foo: bar; title*=iso-8859-1'en'%A3%20rates
       *
       *   Extended notation, using the Unicode characters U+00A3 (POUND SIGN) and U+20AC (EURO SIGN):
       *
       *      foo: bar; title*=UTF-8''%c2%a3%20and%20%e2%82%ac%20rates
       *
       */


      p += strlen(name);
      if(*p == '*'){
         extended = 1;
      }

      p = strchr(p, '=');
      if(p){
         p++;

         // skip any whitespace after name=, ie. name = "
         while(*p==' ' || *p=='\t') p++;

         // if there's a double quote after the equal symbol (=), ie. name*="utf-8....
         if(*p == '"'){
            p++;
            q = strchr(p, '"');

            if(q) *q = '\0';
         }
         else {
            // no " after =, so split on ;
            q = strchr(p, ';');
            if(q) *q = '\0';
         }

         if(extended == 1){
            char *encoding = p;
            q = strchr(p, '\'');
            if(q){
               *q = '\0';
               p = q + 1;
               q = strchr(p, '\'');
               if(q) p = q + 1;
            }

            //decodeURL(p);

            if(strlen(encoding) > 2 && strcasecmp(encoding, "utf-8"))
               utf8_encode(p, strlen(p), resultbuf, resultbuflen-1, encoding);
            else
               snprintf(resultbuf, resultbuflen-1, "%s", p);
         }
         else {
            char puf[SMALLBUFSIZE];

            snprintf(puf, sizeof(puf)-1, "%s", p);
            fixup_encoded_header(puf, sizeof(puf));

            snprintf(resultbuf, resultbuflen-1, "%s", puf);
         }
      }
   }
}
