/*
 * parser_utils.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "misc.h"
#include "decoder.h"
#include "list.h"
#include "boundary.h"
#include "trans.h"
#include "ijc.h"
#include "parser.h"
#include "hash.h"
#include "config.h"
#include "defs.h"


void initState(struct _state *state){
   int i;

   state->message_state = MSG_UNDEF;

   state->is_header = 1;

   /* by default we are a text/plain message */

   state->textplain = 1;
   state->texthtml = 0;
   state->message_rfc822 = 0;

   state->base64 = 0;
   state->utf8 = 0;

   state->qp = 0;

   state->html_comment = 0;

   state->n_token = 0;
   state->n_body_token = 0;
   state->n_chain_token = 0;
   state->n_subject_token = 0;

   state->c_shit = 0;
   state->l_shit = 0;

   state->line_num = 0;

   state->ipcnt = 0;

   state->train_mode = T_TOE;

   memset(state->ip, 0, SMALLBUFSIZE);
   memset(state->hostname, 0, SMALLBUFSIZE);
   memset(state->miscbuf, 0, MAX_TOKEN_LEN);
   memset(state->qpbuf, 0, MAX_TOKEN_LEN);
   memset(state->from, 0, SMALLBUFSIZE);

   state->urls = NULL;

   state->found_our_signo = 0;

   state->has_to_dump = 0;
   state->fd = 0;
   state->num_of_images = 0;
   state->num_of_msword = 0;

   state->boundaries = NULL;

   state->n_attachments = 0;
   state->has_base64 = 0;

   for(i=0; i<MAX_ATTACHMENTS; i++){
      state->attachments[i].size = 0;
      memset(state->attachments[i].type, 0, SMALLBUFSIZE);
   }

   inithash(state->token_hash);
}


void freeState(struct _state *state){
   freeList(state->urls);
   clearhash(state->token_hash, 0);
}


int attachment_by_type(struct _state *state, char *type){
   int i;

   for(i=0; i<MAX_ATTACHMENTS; i++){
      if(strstr(state->attachments[i].type, type))
         return 1;
   }

   return 0;
}


/*
 * extract bondary
 */

int extract_boundary(char *p, struct _state *state){
   char *q;

   p += strlen("boundary");

   q = strchr(p, '"');
   if(q) *q = ' ';

   p = strchr(p, '=');
   if(p){
      p++;
      for(; *p; p++){
         if(isspace(*p) == 0)
            break;
      }
      q = strrchr(p, '"');
      if(q) *q = '\0';

      q = strrchr(p, '\r');
      if(q) *q = '\0';

      q = strrchr(p, '\n');
      if(q) *q = '\0';

      append_boundary(&(state->boundaries), p);

      return 1;
   }

   return 0;
}


void fixupEncodedHeaderLine(char *buf){
   int x;
   char *p, *q, u[SMALLBUFSIZE], puf[MAXBUFSIZE];

   memset(puf, 0, MAXBUFSIZE);

   q = buf;

   do {
      q = split_str(q, " ", u, SMALLBUFSIZE-1);
      x = 0;

      p = strcasestr(u, "?B?");
      if(p){
         decodeBase64(p+3);
         x = 1;
      }
      else if((p = strcasestr(u, "?Q?"))){
         decodeQP(p+3);
         x = 1;
      }

      if(x == 1){
         if(strcasestr(u, "=?utf-8?")) decodeUTF8(p+3);
         strncat(puf, p+3, MAXBUFSIZE-1);
      }
      else {
         strncat(puf, u, MAXBUFSIZE-1);
      }

      strncat(puf, " ", MAXBUFSIZE-1);

   } while(q);

   snprintf(buf, MAXBUFSIZE-1, "%s", puf);
}


void fixupSoftBreakInQuotedPritableLine(char *buf, struct _state *state){
   int i=0;
   char *p, puf[MAXBUFSIZE];

   if(strlen(state->qpbuf) > 0){
      memset(puf, 0, MAXBUFSIZE);
      strncpy(puf, state->qpbuf, MAXBUFSIZE-1);
      strncat(puf, buf, MAXBUFSIZE-1);

      memset(buf, 0, MAXBUFSIZE);
      memcpy(buf, puf, MAXBUFSIZE);

      memset(state->qpbuf, 0, MAX_TOKEN_LEN);
   }

   if(buf[strlen(buf)-2] == '='){
      buf[strlen(buf)-2] = '\0';
      i = 1;
   }

   if(buf[strlen(buf)-3] == '='){
      buf[strlen(buf)-3] = '\0';
      i = 1;
   }

   if(i == 1){
      p = strrchr(buf, ' ');
      if(p){
         memset(state->qpbuf, 0, MAX_TOKEN_LEN);
         if(strlen(p) < MAX_TOKEN_LEN-1){
            snprintf(state->qpbuf, MAXBUFSIZE-1, "%s", p);
            *p = '\0';
         }

      }
   }
}


void fixupBase64EncodedLine(char *buf, struct _state *state){
   char *p, puf[MAXBUFSIZE];

   if(strlen(state->miscbuf) > 0){
      memset(puf, 0, MAXBUFSIZE);
      strncpy(puf, state->miscbuf, MAXBUFSIZE-1);
      strncat(puf, buf, MAXBUFSIZE-1);

      memset(buf, 0, MAXBUFSIZE);
      memcpy(buf, puf, MAXBUFSIZE);

      memset(state->miscbuf, 0, MAX_TOKEN_LEN);
   }

   if(buf[strlen(buf)-1] != '\n'){
      p = strrchr(buf, ' ');
      if(p){
         strncpy(state->miscbuf, p+1, MAX_TOKEN_LEN-1);
         *p = '\0';
      }
   }
}


/*
 * translate buffer
 */

void translateLine(unsigned char *p, struct _state *state){
   int url=0;
   unsigned char *q=NULL, *P=p;

   for(; *p; p++){

      /* save position of '=', 2006.01.05, SJ */

      if(state->qp == 1 && *p == '='){
         q = p;
      }

      if( (state->message_state == MSG_RECEIVED || state->message_state == MSG_FROM || state->message_state == MSG_TO) && *p == '@'){ continue; }

      if(state->message_state == MSG_SUBJECT && *p == '%'){ continue; }

      if(state->message_state == MSG_CONTENT_TYPE && *p == '_' ){ continue; }

      if(state->message_state != MSG_BODY && (*p == '.' || *p == '-') ){ continue; }

      if(strncasecmp((char *)p, "http://", 7) == 0){ p += 7; url = 1; continue; }
      if(strncasecmp((char *)p, "https://", 8) == 0){ p += 8; url = 1; continue; }

      if(url == 1 && (*p == '.' || *p == '-' || *p == '_' || isalnum(*p)) ) continue;

      if(url == 1) url = 0;

      if(delimiter_characters[(unsigned int)*p] != ' ' || isalnum(*p) == 0)
         *p = ' ';
      else {
      #ifndef HAVE_CASE
         *p = tolower(*p);
      #endif
      }

   }

   /* restore the soft break in quoted-printable parts, 2006.01.05, SJ */

   if(state->qp == 1 && q && (q > P + strlen((char*)P) - 3))
     *q = '=';

}


/*
 * reassemble 'V i a g r a' to 'Viagra'
 */

void reassembleToken(char *p){
   int i, k=0;

   for(i=0; i<strlen(p); i++){

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


/*
 * degenerate a token
 */


void degenerateToken(unsigned char *p){
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
}


/*
 * count the invalid characters (ie. garbage on your display) in the buffer
 */

int countInvalidJunkCharacters(char *p, int replace_junk){
   int i=0;

   for(; *p; p++){
      if(invalid_junk_characters[(unsigned char)*p] == *p){
         i++;
         if(replace_junk == 1) *p = JUNK_REPLACEMENT_CHAR;
      }
   }

   return i;
}


/*
 * detect Chinese, Japan, Korean, ... lines
 */

int countInvalidJunkLines(char *p){
   int i=0;

   if(*p == '' && *(p+1) == '$' && *(p+2) == 'B'){
      for(; *p; p++){
         if(*p == '' && *(p+1) == '(' && *(p+2) == 'B')
            i++;
      }
   }

   return i;
}


/*
 * is this a hexadecimal numeric string?
 */

int isHexNumber(char *p){
   for(; *p; p++){
      if(!(*p == '-' || (*p >= 0x30 && *p <= 0x39) || (*p >= 0x41 && *p <= 0x47) || (*p >= 0x61 && *p <= 0x67)) )
         return 0;
   }

   return 1;
}


void fixURL(char *url){
   char *p, *q, m[MAX_TOKEN_LEN], fixed_url[MAXBUFSIZE];
   int i, dots=0;
   struct in_addr addr;

   /* chop trailing dot */

   if(url[strlen(url)-1] == '.')
      url[strlen(url)-1] = '\0';

   memset(fixed_url, 0, MAXBUFSIZE);

   if((strncasecmp(url, "http://", 7) == 0 || strncasecmp(url, "https://", 8) == 0) ){
      p = url;

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

      dots = countCharacterInBuffer(p, '.');
      if(dots < 1)
         return;

      strncpy(fixed_url, "URL*", MAXBUFSIZE-1);

      /* is it a numeric IP-address? */

      if(inet_aton(p, &addr)){
         addr.s_addr = ntohl(addr.s_addr);
         strncat(fixed_url, inet_ntoa(addr), MAXBUFSIZE-1);
         strcpy(url, fixed_url);
      }
      else {
         for(i=0; i<=dots; i++){
            q = split(p, '.', m, MAX_TOKEN_LEN-1);
            if(i>dots-2){
               strncat(fixed_url, m, MAXBUFSIZE-1);
               if(i < dots)
                  strncat(fixed_url, ".", MAXBUFSIZE-1);
            }
            p = q;
         }

         /* if it does not contain a single dot, the rest of the URL may be
            in the next line or it is a fake one, anyway skip, 2006.04.06, SJ
          */

         if(countCharacterInBuffer(fixed_url, '.') != 1)
            memset(url, 0, MAXBUFSIZE);
         else {
            for(i=4; i<strlen(fixed_url); i++)
               fixed_url[i] = tolower(fixed_url[i]);

            strcpy(url, fixed_url);
         }
      }
   }

}


/*
 * fix a long FQDN
 */

void fixFQDN(char *fqdn){
   char *p, *q, m[MAX_TOKEN_LEN], fixed_fqdn[MAXBUFSIZE];
   int i, dots=0;

   /* chop trailing dot */

   if(fqdn[strlen(fqdn)-1] == '.')
      fqdn[strlen(fqdn)-1] = '\0';

   memset(fixed_fqdn, 0, MAXBUFSIZE);

   p = fqdn;

   dots = countCharacterInBuffer(p, '.');
   if(dots < 1)
      return;

   for(i=0; i<=dots; i++){
      q = split(p, '.', m, MAX_TOKEN_LEN-1);
      if(i>dots-2){
         strncat(fixed_fqdn, m, MAXBUFSIZE-1);
         if(i < dots)
             strncat(fixed_fqdn, ".", MAXBUFSIZE-1);
      }
      p = q;
   }

   strcpy(fqdn, fixed_fqdn);
}


/*
 *  extract the .tld from a name (URL, FQDN, ...)
 */

void getTLDFromName(char *name){
   char *p, fixed_name[SMALLBUFSIZE];;

   p = strrchr(name, '.');

   if(p){
      snprintf(fixed_name, SMALLBUFSIZE-1, "URL*%s", p+1);
      strcpy(name, fixed_name);
   }

}


int isItemOnList(char *item, char *list){
   char *p, *q, w[SMALLBUFSIZE], my_list[SMALLBUFSIZE];

   if(!item) return 0;

   snprintf(my_list, SMALLBUFSIZE-1, "127.,192.168.,172.16.,10.,%s", list);

   p = my_list;

   do {
      p = split(p, ',', w, SMALLBUFSIZE-1);

      trimBuffer(w);

      if(strlen(w) > 2){

         if(w[strlen(w)-1] == '$'){
            q = item + strlen(item) - strlen(w) + 1;
            if(strncasecmp(q, w, strlen(w)-1) == 0)
               return 1;
         }
         else if(strcasestr(item, w))
            return 1;

      }

   } while(p);

   return 0;
}


