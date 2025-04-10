/*
 * parser_utils.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <clapf.h>
#include "trans.h"
#include "ijc.h"


void init_state(struct parser_state *state){
   memset((char*)state, 0, sizeof(*state)); // sizeof(state) is only 8 bytes!

   state->tre = '-';

   state->message_state = MSG_UNDEF;

   state->is_header = 1;
   state->is_1st_header = 1;

   state->textplain = 1; /* by default we are a text/plain message */
   state->texthtml = 0;
   state->attachment = -1;
   state->fd = -1;
   state->b64fd = -1;

   inithash(state->boundaries);
   inithash(state->token_hash);
   inithash(state->url);

   state->train_mode = T_TOE;
}


long get_local_timezone_offset(){
   time_t t = time(NULL);
   struct tm lt = {0};
   localtime_r(&t, &lt);
   return lt.tm_gmtoff;
}


int is_hex_number(char *p){
   for(; *p; p++){
      if(!(*p == '-' || *p == ' ' || (*p >= 0x30 && *p <= 0x39) || (*p >= 0x41 && *p <= 0x46) || (*p >= 0x61 && *p <= 0x66)) )
         return 0;
   }

   return 1;
}


int extract_boundary(char *p, struct parser_state *state){
   char *q, *q2;

   p += strlen("boundary");

   q = strchr(p, '"');
   if(q) *q = ' ';

   /*
    * I've seen an idiot spammer using the following boundary definition in the header:
    *
    * Content-Type: multipart/alternative;
    *     boundary=3D"b1_52b92b01a943615aff28b7f4d2f2d69d"
    */

   if(strncmp(p, "=3D", 3) == 0){
      *(p+3) = '=';
      p += 3;
   }

   p = strchr(p, '=');
   if(p){
      p++;
      for(; *p; p++){
         if(isspace(*p) == 0)
            break;
      }

      q2 = strchr(p, ';');
      if(q2) *q2 = '\0';

      q = strrchr(p, '"');
      if(q) *q = '\0';

      q = strrchr(p, '\r');
      if(q) *q = '\0';

      q = strrchr(p, '\n');
      if(q) *q = '\0';

      addnode(state->boundaries, p, 0, 0);

      if(q2) *q2 = ';';

      return 1;
   }

   return 0;
}


void fixupEncodedHeaderLine(char *buf, int buflen){
   char *sb, *sq, *p, *q, *r, *s, *e, *start, *end;
   char v[SMALLBUFSIZE], puf[MAXBUFSIZE], encoding[SMALLBUFSIZE], tmpbuf[2*SMALLBUFSIZE];
   int need_encoding, ret;

   if(buflen < 5) return;

   memset(puf, 0, sizeof(puf));

   q = buf;

   do {
      q = split_str(q, " ", v, sizeof(v)-1);

      p = v;

      memset(encoding, 0, sizeof(encoding));

      do {
         start = strstr(p, "=?");
         if(start){
            *start = '\0';
            if(strlen(p) > 0){
               strncat(puf, p, sizeof(puf)-1);
            }

            start++;

            e = strchr(start+2, '?');
            if(e){
               *e = '\0';
               snprintf(encoding, sizeof(encoding)-1, "%s", start+1);
               *e = '?';
            }

            s = NULL;
            sb = strcasestr(start, "?B?"); if(sb) s = sb;
            sq = strcasestr(start, "?Q?"); if(sq) s = sq;

            if(s){
               end = strstr(s+3, "?=");
               if(end){
                  *end = '\0';

                  if(sb){ decodeBase64(s+3); }
                  if(sq){ decodeQP(s+3); r = s + 3; for(; *r; r++){ if(*r == '_') *r = ' '; } }

                  /* encode everything if it's not utf-8 encoded */

                  need_encoding = 0;
                  ret = ERR;

                  if(strlen(encoding) > 2 && strcasecmp(encoding, "utf-8")){
                     need_encoding = 1;
                     ret = utf8_encode(s+3, strlen(s+3), &tmpbuf[0], sizeof(tmpbuf), encoding);
                  }

                  if(need_encoding == 1 && ret == OK)
                     strncat(puf, tmpbuf, sizeof(puf)-1);
                  else
                     strncat(puf, s+3, sizeof(puf)-1);

                  p = end + 2;
               }
            }
            else {
               strncat(puf, start, sizeof(puf)-1);

               break;
            }
         }
         else {
            strncat(puf, p, sizeof(puf)-1);
            break;
         }

      } while(p);

      if(q) strncat(puf, " ", sizeof(puf)-1);

   } while(q);

   snprintf(buf, buflen-1, "%s", puf);
}


void fixupSoftBreakInQuotedPritableLine(char *buf, struct parser_state *state){
   int i=0;
   char *p, puf[MAXBUFSIZE];

   if(strlen(state->qpbuf) > 0){
      memset(puf, 0, sizeof(puf));
      snprintf(puf, sizeof(puf)-1, "%s%s", state->qpbuf, buf);
      snprintf(buf, MAXBUFSIZE-1, "%s", puf);
      memset(state->qpbuf, 0, MAX_TOKEN_LEN);
   }

   if(buf[strlen(buf)-1] == '='){
      buf[strlen(buf)-1] = '\0';
      i = 1;
   }

   if(i == 1){
      p = strrchr(buf, ' ');
      if(p){
         memset(state->qpbuf, 0, MAX_TOKEN_LEN);
         if(strlen(p) < MAX_TOKEN_LEN-1){
            memcpy(&(state->qpbuf[0]), p, MAX_TOKEN_LEN-1);
            *p = '\0';
         }

      }
   }
}


void fixupBase64EncodedLine(char *buf, struct parser_state *state){
   char *p, puf[MAXBUFSIZE];

   if(strlen(state->miscbuf) > 0){
      memset(puf, 0, sizeof(puf));
      strncpy(puf, state->miscbuf, sizeof(puf)-1);
      strncat(puf, buf, sizeof(puf)-1);

      memset(buf, 0, MAXBUFSIZE);
      memcpy(buf, puf, MAXBUFSIZE);

      memset(state->miscbuf, 0, MAX_TOKEN_LEN);
   }

   if(buf[strlen(buf)-1] != '\n'){
      p = strrchr(buf, ' ');
      if(p){
         memcpy(&(state->miscbuf[0]), p+1, MAX_TOKEN_LEN-1);
         *p = '\0';
      }
   }
}


void remove_html(char *buf, struct parser_state *state){
   char *s, puf[MAXBUFSIZE], html[SMALLBUFSIZE];
   int k=0, pos=0;
   size_t j=0;

   memset(puf, 0, MAXBUFSIZE);
   memset(html, 0, SMALLBUFSIZE);

   s = buf;

   for(; *s; s++){
      if(*s == '<'){
         state->htmltag = 1;
         puf[k] = ' ';
         k++;

         memset(html, 0, sizeof(html)); j=0;

         pos = 0;
      }

      if(state->htmltag == 1){

         if(j == 0 && *s == '!'){
            state->skip_html = 1;
         }

         if(state->skip_html == 0){
            if(*s != '>' && *s != '<' && *s != '"'){
               html[j] = tolower(*s);
               if(j < sizeof(html)-10) j++;
            }

            if(isspace(*s)){
               if(j > 0){
                  //k += appendHTMLTag(puf, html, pos, state);
                  //memset(html, 0, sizeof(html)); j=0;
               }
               pos++;
            }
         }
      }
      else {
         if(state->style == 0){
            puf[k] = *s;
            k++;
         }
      }

      if(*s == '>'){
         state->htmltag = 0;
         state->skip_html = 0;

         if(j > 0){
            strncat(html, " ", sizeof(html)-1);
            k += append_html_tag(puf, html, pos, state);
            memset(html, 0, sizeof(html)); j=0;
         }
      }

   }

   if(j > 0){ k += append_html_tag(puf, html, pos, state); }

   strcpy(buf, puf);
}


int append_html_tag(char *buf, char *htmlbuf, int pos, struct parser_state *state){
   char *p, *q;
   int len=0;

   if(pos <= 1 && strncmp(htmlbuf, "style ", 6) == 0){ state->style = 1; return 0; }
   if(pos == 0 && strncmp(htmlbuf, "/style ", 7) == 0){ state->style = 0; return 0; }

   // skip ....html xmlns="http://www.w3.org/1999/xhtml...
   if(strncmp(htmlbuf, "html ", 5) == 0) return 0;

   if(strlen(htmlbuf) == 0) return 0;

   // skip all html tokens, except alt, cid:.... and http(s)://...

   p = strstr(htmlbuf, "http://");
   if(!p) p = strstr(htmlbuf, "https://");

   if(p){ q = strchr(p, ' '); if(q) *q = '\0'; }

   if(!p){ p = strstr(htmlbuf, "cid:"); if(p){ strncat(buf, "src cid ", MAXBUFSIZE-2); return 8; } }

   if(!p){ p = strstr(htmlbuf, "alt="); if(p) p += 4; }
   if(p){
      len = strlen(p);
      strncat(buf, p, MAXBUFSIZE-2);
      strncat(buf, " ", MAXBUFSIZE-2);

      return len+1;
   }

   return 0;
}


void translate_line(unsigned char *p, struct parser_state *state){
   int url=0;
   int has_url = 0;

   if(strcasestr((char *)p, "http://") || strcasestr((char *)p, "https://")) has_url = 1;

   for(; *p; p++){

      if( (state->message_state == MSG_RECEIVED || state->message_state == MSG_FROM || state->message_state == MSG_TO || state->message_state == MSG_CC || state->message_state == MSG_RECIPIENT) && (*p == '@' || *p == '-') ){ continue; }

      if( (state->message_state == MSG_FROM || state->message_state == MSG_TO || state->message_state == MSG_CC || state->message_state == MSG_RECIPIENT) && (*p == '_' || *p == '\'' || *p == '&') ){ continue; }

      if(state->message_state == MSG_SUBJECT && (*p == '%' || *p == '_' || *p == '&') ){ continue; }

      if(state->message_state == MSG_CONTENT_TYPE && *p == '_' ){ continue; }

      if(*p == '.'){ continue; }

      if(has_url == 1){
         if(strncasecmp((char *)p, "http://", 7) == 0){ p += 7; url = 1; continue; }
         if(strncasecmp((char *)p, "https://", 8) == 0){ p += 8; url = 1; continue; }

         if(url == 1 && (*p == '.' || *p == '-' || *p == '_' || *p == '/' || *p == '%' || *p == '?' || isalnum(*p)) ) continue;
         if(url == 1) url = 0;
      }

      if(*p == '@') *p = 'X';

      if(delimiter_characters[(unsigned int)*p] != ' ') *p = ' ';

      else if(*p < 128) *p = tolower(*p);
      else if(*p == 0xC3) { p++; if(*p >= 0x81 && *p <= 0x9F) *p = *p + 32; }
      else if(*p == 0xC5) { p++; if(*p == 0x90 || *p == 0xB0) *p = *p + 1;  }

      /* we MUSTN'T convert it to lowercase in the 'else' case, because it breaks utf-8 encoding! */

   }

}


void split_email_address(char *s){
   for(; *s; s++){
      if(*s == '@' || *s == '.' || *s == '+' || *s == '-' || *s == '_') *s = ' ';
   }
}


int does_it_seem_like_an_email_address(char *email){
   char *p;

   if(email == NULL) return 0;

   if(strlen(email) < 5) return 0;

   p = strchr(email, '@');
   if(!p) return 0;

   if(strlen(p+1) < 3) return 0;

   if(!strchr(p+1, '.')) return 0;

   return 1;
}


/*
 * reassemble 'V i a g r a' to 'Viagra'
 */

void reassemble_token(char *p){
   int k=0;

   for(size_t i=0; i<strlen(p); i++){

      if(isspace(*(p+i-1)) && !isspace(*(p+i)) && isspace(*(p+i+1)) && !isspace(*(p+i+2)) && isspace(*(p+i+3)) && !isspace(*(p+i+4)) && isspace(*(p+i+5)) ){
         p[k] = *(p+i); k++;
         p[k] = *(p+i+2); k++;
         p[k] = *(p+i+4); k++;

         i += 5;
      }
      else {
         p[k] = *(p+i);
         k++;
      }
   }

   p[k] = '\0';
}


void degenerate_token(unsigned char *p){
   int i=1, d=0, dp=0;
   unsigned char *s;

   /* quit if this the string does not end with a punctuation character */

   if(!ispunct(*(p+strlen((char *)p)-1)))
      return;

   s = p;

   for(; *p; p++){
      if(ispunct(*p)){
         d = i;

         if(!ispunct(*(p-1)))
            dp = d;
      }
      else
         d = dp = i;

      i++;
   }

   *(s+dp) = '\0';

   if(*(s+dp-1) == '.' || *(s+dp-1) == '!' || *(s+dp-1) == '?') *(s+dp-1) = '\0';
}


void fix_URL(char *url){
   char *p, *q, m[MAX_TOKEN_LEN], fixed_url[MAXBUFSIZE];
   int dots=0, result;
   struct in_addr addr;

   /* chop trailing dot */

   if(url[strlen(url)-1] == '.')
      url[strlen(url)-1] = '\0';

   memset(fixed_url, 0, sizeof(fixed_url));

   if((strncasecmp(url, "http://", 7) == 0 || strncasecmp(url, "https://", 8) == 0) ){
      p = url;

      if(strncasecmp(p, "cid:", 4) == 0) p += 4;
      if(strncasecmp(p, "http://", 7) == 0) p += 7;
      if(strncasecmp(p, "https://", 8) == 0) p += 8;

      /* skip anything after the host part, 2006.12.11, SJ */
      q = strchr(p, '/');
      if(q)
         *q = '\0';

      /*
         http://www.ajandekkaracsonyra.hu/email.php?page=email&cmd=unsubscribe&email=yy@xxxx.kom is
         chopped to www.ajandekkaracsonyra.hu at this point, 2006.12.15, SJ
       */

      dots = count_character_in_buffer(p, '.');
      if(dots < 1)
         return;

      strncpy(fixed_url, "URL*", sizeof(fixed_url)-1);

      /* is it a numeric IP-address? */

      if(inet_aton(p, &addr)){
         addr.s_addr = ntohl(addr.s_addr);
         strncat(fixed_url, inet_ntoa(addr), sizeof(fixed_url)-1);
         strncat(fixed_url, " ", sizeof(fixed_url)-1);
         strcpy(url, fixed_url);
      }
      else {
         for(int i=0; i<=dots; i++){
            q = split(p, '.', m, sizeof(m)-1, &result);
            if(i>dots-2){
               strncat(fixed_url, m, sizeof(fixed_url)-1);
               if(i < dots)
                  strncat(fixed_url, ".", sizeof(fixed_url)-1);
            }
            p = q;
         }

         strncat(fixed_url, " ", sizeof(fixed_url)-1);

         /* if it does not contain a single dot, the rest of the URL may be
            in the next line or it is a fake one, anyway skip, 2006.04.06, SJ
          */

         if(count_character_in_buffer(fixed_url, '.') != 1)
            memset(url, 0, MAXBUFSIZE);
         else {
            for(size_t i=4; i<strlen(fixed_url); i++)
               fixed_url[i] = tolower(fixed_url[i]);

            strcpy(url, fixed_url);
         }
      }
   }

}


/*
 * fix a long FQDN
 */

void fix_FQDN(char *fqdn){
   char *p, *q, m[MAX_TOKEN_LEN], fixed_fqdn[MAXBUFSIZE];
   int i, dots=0, x;

   /* chop trailing dot */

   if(fqdn[strlen(fqdn)-1] == '.')
      fqdn[strlen(fqdn)-1] = '\0';

   memset(fixed_fqdn, 0, sizeof(fixed_fqdn));

   p = fqdn;

   dots = count_character_in_buffer(p, '.');
   if(dots < 1)
      return;

   for(i=0; i<=dots; i++){
      q = split(p, '.', m, sizeof(m)-1, &x);
      if(i>dots-2){
         strncat(fixed_fqdn, m, sizeof(fixed_fqdn)-1);
         if(i < dots)
             strncat(fixed_fqdn, ".", sizeof(fixed_fqdn)-1);
      }
      p = q;
   }

   strcpy(fqdn, fixed_fqdn);
}


/*
 *  extract the .tld from a name (URL, FQDN, ...)
 */

void get_tld_from_name(char *name){
   char *p, fixed_name[SMALLBUFSIZE];;

   p = strrchr(name, '.');

   if(p){
      snprintf(fixed_name, sizeof(fixed_name)-1, "URL*%s", p+1);
      strcpy(name, fixed_name);
   }

}


/*
 * count the invalid CJK (=Chinese, Japanese, Korean) characters
 */

int count_invalid_junk_characters(char *p, int replace_junk){
   int count=0;

   for(; *p; p++){
      if(invalid_junk_characters[(unsigned char)*p] == *p){
         count++;
         if(replace_junk == 1) *p = JUNK_REPLACEMENT_CHAR;
      }
   }

   return count;
}


/*
 * detect CJK lines
 */

int count_invalid_junk_lines(char *p){
   int count=0;

   if(*p == '' && *(p+1) == '$' && *(p+2) == 'B'){
      for(; *p; p++){
         if(*p == '' && *(p+1) == '(' && *(p+2) == 'B')
            count++;
      }
   }

   return count;
}


char *determine_attachment_type(char *filename, char *type){
   char *p;

   if(strncasecmp(type, "text/", strlen("text/")) == 0) return "text,";
   if(strncasecmp(type, "image/", strlen("image/")) == 0) return "image,";
   if(strncasecmp(type, "audio/", strlen("audio/")) == 0) return "audio,";
   if(strncasecmp(type, "video/", strlen("video/")) == 0) return "video,";
   if(strncasecmp(type, "text/x-card", strlen("text/x-card")) == 0) return "vcard,";

   if(strncasecmp(type, "application/pdf", strlen("application/pdf")) == 0) return "pdf,";

   if(strncasecmp(type, "application/ms-tnef", strlen("application/ms-tnef")) == 0) return "tnef,";
   if(strncasecmp(type, "application/msword", strlen("application/msword")) == 0) return "word,";

   // a .csv file has the same type
   if(strncasecmp(type, "application/vnd.ms-excel", strlen("application/vnd.ms-excel")) == 0) return "excel,";

   if(strncasecmp(type, "application/vnd.ms-powerpoint", strlen("application/vnd.ms-powerpoint")) == 0) return "powerpoint,";

   if(strncasecmp(type, "application/vnd.visio", strlen("application/vnd.visio")) == 0) return "visio,";

   if(strncasecmp(type, "application/vnd.openxmlformats-officedocument.wordprocessingml.document", strlen("application/vnd.openxmlformats-officedocument.wordprocessingml.document")) == 0) return "word,";
   if(strncasecmp(type, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", strlen("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet")) == 0) return "excel,";
   if(strncasecmp(type, "application/vnd.openxmlformats-officedocument.presentationml.presentation", strlen("application/vnd.openxmlformats-officedocument.presentationml.presentation")) == 0) return "powerpoint,";

   if(strncasecmp(type, "application/x-shockwave-flash", strlen("application/x-shockwave-flash")) == 0) return "flash,";

   if(strcasestr(type, "opendocument")) return "odf,";



   if(strncasecmp(type, "application/", 12) == 0){

      p = strrchr(filename, '.');
      if(p){
         p++;

         if(strncasecmp(p, "pdf", 3) == 0) return "pdf,";

         if(strncasecmp(p, "zip", 3) == 0) return "compressed,";
         if(strncasecmp(p, "rar", 3) == 0) return "compressed,";

         // tar.gz has the same type
         if(strncasecmp(p, "x-gzip", 3) == 0) return "compressed,";

         if(strncasecmp(p, "rtf", 3) == 0) return "word,";
         if(strncasecmp(p, "doc", 3) == 0) return "word,";
         if(strncasecmp(p, "docx", 4) == 0) return "word,";
         if(strncasecmp(p, "xls", 3) == 0) return "excel,";
         if(strncasecmp(p, "xlsx", 4) == 0) return "excel,";
         if(strncasecmp(p, "ppt", 3) == 0) return "powerpoint,";
         if(strncasecmp(p, "pptx", 4) == 0) return "powerpoint,";

         if(strncasecmp(p, "png", 3) == 0) return "image,";
         if(strncasecmp(p, "gif", 3) == 0) return "image,";
         if(strncasecmp(p, "jpg", 3) == 0) return "image,";
         if(strncasecmp(p, "jpeg", 4) == 0) return "image,";
         if(strncasecmp(p, "tiff", 4) == 0) return "image,";
      }
   }

   return "other,";
}


char *get_attachment_extractor_by_filename(char *filename){
   char *p;

   if(strcasecmp(filename, "winmail.dat") == 0) return "tnef";

   p = strrchr(filename, '.');
   if(!p) return "other";

   if(strcasecmp(p, ".pdf") == 0) return "pdf";
   if(strcasecmp(p, ".zip") == 0) return "zip";
   if(strcasecmp(p, ".gz") == 0) return "gzip";
   if(strcasecmp(p, ".rar") == 0) return "rar";
   if(strcasecmp(p, ".odt") == 0) return "odf";
   if(strcasecmp(p, ".odp") == 0) return "odf";
   if(strcasecmp(p, ".ods") == 0) return "odf";
   if(strcasecmp(p, ".doc") == 0) return "doc";
   if(strcasecmp(p, ".docx") == 0) return "docx";
   if(strcasecmp(p, ".xls") == 0) return "xls";
   if(strcasecmp(p, ".xlsx") == 0) return "xlsx";
   if(strcasecmp(p, ".ppt") == 0) return "ppt";
   if(strcasecmp(p, ".pptx") == 0) return "pptx";
   if(strcasecmp(p, ".rtf") == 0) return "rtf";
   if(strcasecmp(p, ".txt") == 0) return "text";
   if(strcasecmp(p, ".csv") == 0) return "text";

   return "other";
}


int base64_decode_attachment_buffer(char *p, unsigned char *b, int blen){
   int b64len=0;
   char puf[2*SMALLBUFSIZE];

   do {
      p = split_str(p, "\n", puf, sizeof(puf)-1);
      trim_buffer(puf);
      b64len += decode_base64_to_buffer(puf, strlen(puf), b+b64len, blen);
   } while(p);

   return b64len;
}


void remove_stripped_attachments(struct parser_state *state){
   int i;

   for(i=1; i<=state->n_attachments; i++) unlink(state->attachments[i].internalname);
}


int has_octet_stream(struct parser_state *state){
   int i;

   for(i=1; i<=state->n_attachments; i++){
      if(
          strstr(state->attachments[i].type, "application/octet-stream") ||
          strstr(state->attachments[i].type, "application/pdf") ||
          strstr(state->attachments[i].type, "application/vnd.ms-excel") ||
          strstr(state->attachments[i].type, "application/msword") ||
          strstr(state->attachments[i].type, "application/rtf") ||
          strstr(state->attachments[i].type, "application/x-zip-compressed")

      ) return 1;
   }

   return 0;
}


int has_image_attachment(struct parser_state *state){
   int i;

   for(i=1; i<=state->n_attachments; i++){
      if(strstr(state->attachments[i].type, "image/")) return 1;
   }

   return 0;
}
