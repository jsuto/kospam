/*
 * parser.c, 2010.05.13, SJ
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
#include "parser.h"
#include "hash.h"
#include "config.h"
#include "defs.h"


/*
 * parse the message into tokens and return the pointer
 */

struct _state parseMessage(struct session_data *sdata, struct __config *cfg){
   FILE *f;
   int skipped_header = 0, found_clapf_signature = 0;
   char *p, *q;
   char buf[MAXBUFSIZE], tumbuf[SMALLBUFSIZE];
   struct _state state;

   init_state(&state);

   f = fopen(sdata->ttmpfile, "r");
   if(!f){
      syslog(LOG_PRIORITY, "%s: cannot open", sdata->ttmpfile);
      return state;
   }


   snprintf(tumbuf, SMALLBUFSIZE-1, "%sTUM", cfg->clapf_header_field);

   while(fgets(buf, MAXBUFSIZE-1, f)){

      if(sdata->training_request == 0 || found_clapf_signature == 1){
         parseLine(buf, &state, sdata, cfg);
         if(strncmp(buf, tumbuf, strlen(tumbuf)) == 0) state.train_mode = T_TUM;
      }

      if(found_clapf_signature == 0 && sdata->training_request == 1){

         if(buf[0] == '\n' || (buf[0] == '\r' && buf[1] == '\n') ){
            skipped_header = 1;
         }

         if(skipped_header == 1){
            q = strstr(buf, "Received: ");
            if(q){
               trimBuffer(buf);
               p = strchr(buf, ' ');
               if(p){
                  p++;
                  if(isValidClapfID(p)){
                     snprintf(sdata->clapf_id, SMALLBUFSIZE-1, "%s", p);
                     if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: found id in training request: *%s*", sdata->ttmpfile, p);
                     found_clapf_signature = 1;
                  }
               }
            }
         }
      }

   }


   fclose(f);

   free_boundary(state.boundaries);

   return state;
}


/*
 * parse buffer
 */

int parseLine(char *buf, struct _state *state, struct session_data *sdata, struct __config *cfg){
   char *p, *q, *c, huf[MAXBUFSIZE], puf[MAXBUFSIZE], muf[MAXBUFSIZE], tuf[MAXBUFSIZE], u[SMALLBUFSIZE], token[MAX_TOKEN_LEN], phrase[MAX_TOKEN_LEN];
   int x, b64_len;

   memset(token, 0, MAX_TOKEN_LEN);

   state->line_num++;

   /* undefined message state */
   if(state->is_header == 1 && strchr(buf, ':')) state->message_state = MSG_UNDEF;

   /* skip empty lines */

   if(buf[0] == '\r' || buf[0] == '\n'){
      state->message_state = MSG_BODY;

      if(state->is_header == 1) state->is_header = 0;

      if(cfg->debug == 1) printf("\n");

      return 0;
   }

   /* check for our anti backscatter signo, SJ */

   if(sdata->need_signo_check == 1){
      if(strncmp(buf, cfg->our_signo, strlen(cfg->our_signo)) == 0)
         state->found_our_signo = 1;
   }


   if(strncasecmp(buf, "Content-Type:", strlen("Content-Type:")) == 0) state->message_state = MSG_CONTENT_TYPE;
   else if(strncasecmp(buf, "Content-Transfer-Encoding:", strlen("Content-Transfer-Encoding:")) == 0) state->message_state = MSG_CONTENT_TRANSFER_ENCODING;
   else if(strncasecmp(buf, "Content-Disposition:", strlen("Content-Disposition:")) == 0) state->message_state = MSG_CONTENT_DISPOSITION;


   /* header checks */

   if(state->is_header == 1){

      if(strncmp(buf, "Received: from ", strlen("Received: from ")) == 0) state->message_state = MSG_RECEIVED;
      else if(strncmp(buf, "From:", strlen("From:")) == 0) state->message_state = MSG_FROM;
      else if(strncmp(buf, "To:", 3) == 0) state->message_state = MSG_TO;
      else if(strncmp(buf, "Subject:", strlen("Subject:")) == 0) state->message_state = MSG_SUBJECT;


      if(state->message_state == MSG_FROM){

         p = strchr(buf+5, ' ');
         if(p) p = buf + 6;
         else p = buf + 5;

         snprintf(state->from, SMALLBUFSIZE-1, "FROM*%s", p);
         trimBuffer(state->from);
      }


      /* extract prefix type */

      memset(puf, 0, MAXBUFSIZE);

      if(!isspace(buf[0])){
         p = strchr(buf, ':');
         if(p)
            memcpy(puf, buf, p-buf);
      }

      /* we are interested in only From:, To:, Subject:, Received:, Content-*: header lines */
      if(state->message_state <= 0) return 0;
   }
   else
      state->cnt_type = 0;


   /* end of header checks */


   if((p = strcasestr(buf, "boundary"))){
      x = extract_boundary(p, state);
   }


   /* Content-type: checking */

   if(state->message_state == MSG_CONTENT_TYPE){

      /* extract Content type */

      p = strchr(buf, ':');
      if(p){
         p++;
         if(*p == ' ' || *p == '\t') p++;
         snprintf(state->attachments[state->n_attachments].type, SMALLBUFSIZE-1, "%s", p);
         trimBuffer(state->attachments[state->n_attachments].type);
         p = strchr(state->attachments[state->n_attachments].type, ';');
         if(p) *p = '\0';

      }

      state->cnt_type = 1;

      /* 2007.04.19, SJ */

      if(strcasestr(buf, "text/plain") ||
         strcasestr(buf, "multipart/mixed") ||
         strcasestr(buf, "multipart/alternative") ||
         strcasestr(buf, "multipart/report") ||
         strcasestr(buf, "message/delivery-status") ||
         strcasestr(buf, "text/rfc822-headers") ||
         strcasestr(buf, "message/rfc822") ||
         strcasestr(buf, "application/ms-tnef")
      ){

             state->textplain = 1;
      }
      else if(strcasestr(buf, "text/html"))
             state->texthtml = 1;
      else
         goto DECOMPOSE;
   }


   /* check for textual base64 encoded part, 2005.03.25, SJ */

   if(state->message_state == MSG_CONTENT_TRANSFER_ENCODING){

      if(strcasestr(buf, "base64")){
         state->base64 = 1;
         state->has_base64 = 1;
      }

      if(strcasestr(buf, "image"))
         state->num_of_images++;

      if(strcasestr(buf, "msword"))
         state->num_of_msword++;
   }


   if(state->base64 == 1 && !strchr(buf, ':'))
      state->base64_lines++;


   /* check for UTF-8 encoding */

   if(strcasestr(buf, "charset") && strcasestr(buf, "UTF-8"))
      state->utf8 = 1;

   if(strcasestr(buf, "charset") && (strcasestr(buf, "ISO-8859-2") || strcasestr(buf, "ISO-8859-1"))  )
      state->iso_8859_2 = 1;

   /* catch encoded stuff in the Subject|From lines, 2007.09.04, SJ */

   if(state->message_state == MSG_SUBJECT || state->message_state == MSG_FROM){
      if(strcasestr(buf, "?iso-8859-2?") || strcasestr(buf, "?iso-8859-1?")) state->iso_8859_2 = 1;
      if(strcasestr(buf, "?utf-8?")) state->utf8 = 1;
   }

   /* check for quoted-printable encoding */

   if(state->message_state == MSG_CONTENT_TRANSFER_ENCODING && strcasestr(buf, "quoted-printable"))
      state->qp = 1;

   if(state->message_state == MSG_CONTENT_TRANSFER_ENCODING && state->textplain == 0 && state->texthtml == 0)
      goto DECOMPOSE;

   if(state->message_state == MSG_CONTENT_DISPOSITION && state->is_header == 1){
      if(state->textplain == 0 && state->texthtml == 0) goto DECOMPOSE;
   }

   /* is it a base64 encoded text? 2006.01.02, SJ */

   if(state->base64_text == 0 && (state->textplain == 1 || state->texthtml == 1) && state->base64 == 1)
      state->base64_text = 1;


   /* skip the boundary itself */

   /* we had strcasestr here, but I saw the "boundary" string only lowercased, 2009.10.10, SJ */

   if(!strstr(buf, "boundary=") && !strstr(buf, "boundary =") && is_boundary(state->boundaries, buf) == 1){
      if(state->has_to_dump == 1){
         close(state->fd);
         if(state->n_attachments < MAX_ATTACHMENTS-1)
            state->n_attachments++;
      }


      state->has_to_dump = 0;

      // TODO: do not let the boundary definition reset the Content-* variables if we are in the header
      state->base64 = 0; state->base64_lines = 0; state->base64_text = 0; state->textplain = 0; state->texthtml = 0;
      state->html_comment = 0;
      state->utf8 = 0; state->iso_8859_2 = 1;
      // state->qp = 0;

      //printf("skipping found boundary: %s", buf);
      return 0;      
   }

   /* end of boundary check */


   /* skip non textual stuff */

   if(state->is_header == 0 && state->textplain == 0 && state->texthtml == 0)
      return 0;

   /* base64 decode buffer, 2005.03.23, SJ */

   if(state->base64 == 1 && state->is_header == 0 && strncmp(buf, "Content-", strlen("Content-")) != 0){
      memset(huf, 0, MAXBUFSIZE);
      b64_len = decodeBase64(buf, huf);
      if(b64_len > 0)
         strncpy(buf, huf, MAXBUFSIZE-1);
   }



   /* handle encoded From:, To: and Subject: lines, 2008.11.24, SJ */

   if(state->message_state == MSG_SUBJECT || state->message_state == MSG_FROM || state->message_state == MSG_TO){
      memset(tuf, 0, MAXBUFSIZE);

      q = buf;

      do {
         q = split_str(q, " ", u, SMALLBUFSIZE-1);
         x = 0;

         p = strcasestr(u, "?B?");
         if(p){
            decodeBase64(p+3, huf); x = 1;
         }
         else if((p = strcasestr(u, "?Q?"))){
            decodeQP(p+3); x = 1;
            snprintf(huf, MAXBUFSIZE-1, "%s", p+3);
         }

         if(x == 1){
            if(strcasestr(u, "=?utf-8?")) decodeUTF8(huf);
            strncat(tuf, huf, MAXBUFSIZE-1); strncat(tuf, " ", MAXBUFSIZE-1);
         }
         else {
            strncat(tuf, u, MAXBUFSIZE-1); strncat(tuf, " ", MAXBUFSIZE-1);
         }

      } while(q);

      snprintf(buf, MAXBUFSIZE-1, "%s", tuf);
   }



   /* skip the first line too, if it's a "From <email address> date" format */

   if(state->line_num == 1 && buf[0] == 'F' && buf[1] == 'r' && buf[2] == 'o' && buf[3] == 'm' && buf[4] == ' ')
      return 0;


   /* fix soft breaks with quoted-printable decoded stuff, 2006.03.01, SJ */

   if(state->qp == 1 && strlen(state->qpbuf) > 0){
      memset(tuf, 0, MAXBUFSIZE);
      strncpy(tuf, state->qpbuf, MAXBUFSIZE-1);
      strncat(tuf, buf, MAXBUFSIZE-1);

      memset(buf, 0, MAXBUFSIZE);
      memcpy(buf, tuf, MAXBUFSIZE);

      memset(state->qpbuf, 0, MAX_TOKEN_LEN);
   }

   if(state->qp == 1 && buf[strlen(buf)-2] == '='){
      q = strrchr(buf, ' ');
      if(q){
         *q = '\0';
         q++;
         memset(state->qpbuf, 0, MAX_TOKEN_LEN);
         if(strlen(q)-2 < MAX_TOKEN_LEN-1)
            strncpy(state->qpbuf, q, strlen(q)-2);
      }
   }

   /* fix base64 stuff if the line does not end with a line break, 2006.03.01, SJ */

   if(state->base64 == 1 && strlen(state->miscbuf) > 0){
      memset(tuf, 0, MAXBUFSIZE);
      strncpy(tuf, state->miscbuf, MAXBUFSIZE-1);
      strncat(tuf, buf, MAXBUFSIZE-1);

      memset(buf, 0, MAXBUFSIZE);
      memcpy(buf, tuf, MAXBUFSIZE);

      memset(state->miscbuf, 0, MAX_TOKEN_LEN);
   }

   if(state->base64 == 1 && buf[strlen(buf)-1] != '\n'){
      q = strrchr(buf, ' ');
      if(q){
         strncpy(state->miscbuf, q+1, MAX_TOKEN_LEN-1);
         *q = '\0';
      }
   }


   /* handle html comments, 2007.06.07, SJ */

   if(state->texthtml == 1){
      if(state->html_comment == 1 && strchr(buf, '>')) state->html_comment = 0;

      if(state->is_header == 0 && strstr(buf, "<!")) state->html_comment = 1;

      if(state->html_comment == 1){
         q = strstr(buf, "<!");
         if(q){
            *q = '\0';

            if(cfg->debug == 1)
               printf("DISCARDED HTML: %s", ++q);
         }
      }
   }


   if(state->message_state == MSG_BODY){
      if(state->qp == 1)   decodeQP(buf);
      if(state->utf8 == 1) decodeUTF8(buf);
   }

   decodeURL(buf);

   decodeHTML(buf);


   /* Chinese, Japan, Korean, ... language detection here */

   x = 0;

   if(buf[0] == '' && buf[1] == '$' && buf[2] == 'B'){
      c = buf;
      for(; *c; c++){
         if(*c == '' && *(c+1) == '(' && *(c+2) == 'B')
            x++;
      }
   }



   /* count invalid junk characters specified in the ijc.h file, 2008.09.08 */
   state->c_shit += countInvalidJunkCharacters(buf, cfg->replace_junk_characters);

   /* skip unless we have an URL, 2006.11.09, SJ */

   if(x > 0) state->l_shit += x;

DECOMPOSE:
   /* skip unnecessary header lines */
   if(state->message_state == MSG_UNDEF && state->is_header == 1) return 0;

   translateLine((unsigned char*)buf, state);

   reassembleToken(buf);

   if(state->is_header == 1) p = strchr(buf, ' ');
   else p = buf;

   if(cfg->debug == 1) printf("%s\n", buf);
   //if(cfg->debug == 1) printf("%d %ld %s\n", state->base64, state->c_shit, buf);

   do {
      p = split(p, DELIMITER, puf, MAXBUFSIZE-1);

      if(strcasestr(puf, "http://") || strcasestr(puf, "https://")){
         q = puf;
         do {
            q = split_str(q, "http://", u, SMALLBUFSIZE-1);
            if(strlen(u) > 2){
               snprintf(muf, MAXBUFSIZE-1, "http://%s", u);

               fixURL(muf);

               addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);
               append_list(&(state->urls), muf);
               state->n_token++;

               /* create a .tld token, suitable for http://whatever.cn/ URIs, 2009.08.05, SJ */

               getTLDFromName(muf);

               addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);
               append_list(&(state->urls), muf);
               state->n_token++;
                
            }
         } while(q);

         continue;
      }


      /* fixup Received: line tokens, 2010.05.13, SJ */

      if(state->message_state == MSG_RECEIVED){
         x = puf[strlen(puf)-1];

         /* 
          * skip Received line token, if
          *    - no punctuation (eg. by, from, esmtp, ...)
          *    - it's on the skipped_received_hosts or skipped_received_ips list
          *    - ends with a number and not a valid IP-address (8.14.3, 6.0.3790.211, ...)
          */

         if(!strchr(puf, '.') || isItemOnList(puf, cfg->skipped_received_hosts) == 1 || isItemOnList(puf, cfg->skipped_received_ips) == 1 || ( (x >= 0x30 && x <= 0x39) && (isDottedIPv4Address(puf) == 0 || countCharacterInBuffer(puf, '.') != 3) ) ){
            continue;
         }


         /* 
          * fill state.ip and state.hostname fields _after_
          * eliminated all entries matched by skipped_received_ips,
          * and skipped_received_hosts.
          * These entries hold the name and address of the host
          * which hands this email to us.
          */

         if(state->ipcnt == 0){

            if(isDottedIPv4Address(puf) == 1){
               snprintf(state->ip, SMALLBUFSIZE-1, "%s", puf);
               state->ipcnt++;
            }
            else {
               snprintf(state->hostname, SMALLBUFSIZE-1, "%s", puf);
            }
         }


         /* truncate long Received: line tokens */

         if(strlen(puf) > MAX_WORD_LEN){
            fixFQDN(puf);

            snprintf(muf, MAXBUFSIZE-1, "%s", puf);
            getTLDFromName(muf);
            addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);
            state->n_token++;
         }

      }


      /* skip too short or long or numeric only tokens */

      if(strlen(puf) < MIN_WORD_LEN || strlen(puf) > MAX_WORD_LEN || isHexNumber(puf))
         continue;

      if(state->message_state == MSG_SUBJECT){
         snprintf(muf, MAXBUFSIZE-1, "Subject*%s", puf);
         state->n_subject_token++;
      }
      else if(state->message_state == MSG_FROM)
         snprintf(muf, MAXBUFSIZE-1, "FROM*%s", puf);
      else if(state->is_header == 1)
         snprintf(muf, MAXBUFSIZE-1, "HEADER*%s", puf);
      else
         snprintf(muf, MAXBUFSIZE-1, "%s", puf);


      if(muf[0] == 0) continue;

      state->n_token++;
      if(state->message_state != MSG_RECEIVED && state->message_state != MSG_FROM) state->n_chain_token++;

      degenerateToken((unsigned char*)muf);


      /* add single token to list */

      addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);


      /* create token pairs, 2007.06.06, SJ */

      if(state->is_header == 0) state->n_body_token++;

      if(((state->is_header == 1 && state->n_chain_token > 1) || state->n_body_token > 1) && strlen(token) >= MIN_WORD_LEN && state->message_state != MSG_CONTENT_TYPE){
         snprintf(phrase, MAX_TOKEN_LEN-1, "%s+%s", token, muf);
         addnode(state->token_hash, phrase, DEFAULT_SPAMICITY, 0);
      }
      snprintf(token, MAX_TOKEN_LEN-1, "%s", muf);

      memset(muf, 0, MAXBUFSIZE);

   } while(p);


   /* do not chain between individual headers, 2007.06.09, SJ */
   if(state->is_header == 1) state->n_chain_token = 0;

   if(state->message_state == MSG_FROM && strlen(state->from) > 3)
      addnode(state->token_hash, state->from, DEFAULT_SPAMICITY, 0);

   return 0;
}


#ifdef HAVE_MAILBUF
struct _state parseBuffer(struct session_data *sdata, struct __config *cfg){
   int skipped_header = 0, found_clapf_signature = 0;
   char *p, *q, *r;
   char buf[MAXBUFSIZE], tumbuf[SMALLBUFSIZE];
   struct _state state;

   init_state(&state);

   if(sdata->mailpos < 10 || sdata->discard_mailbuf == 1){
      syslog(LOG_PRIORITY, "%s: invalid mail buffer", sdata->ttmpfile);
      return state;
   }


   snprintf(tumbuf, SMALLBUFSIZE-1, "%sTUM", cfg->clapf_header_field);

   r = sdata->mailbuf;

   do {
      r = split(r, '\n', buf, MAXBUFSIZE-1);

      if(sdata->training_request == 0 || found_clapf_signature == 1){
         parse(buf, &state, sdata, cfg);
         if(strncmp(buf, tumbuf, strlen(tumbuf)) == 0) state.train_mode = T_TUM;
      }

      if(found_clapf_signature == 0 && sdata->training_request == 1){
         if(buf[0] == 0 || buf[0] == '\r'){
            skipped_header = 1;
         }

         if(skipped_header == 1){
            q = strstr(buf, "Received: ");
            if(q){
               trimBuffer(buf);
               p = strchr(buf, ' ');
               if(p){
                  p++;
                  if(isValidClapfID(p)){
                     snprintf(sdata->clapf_id, SMALLBUFSIZE-1, "%s", p);
                     if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: found id in training request: *%s*", sdata->ttmpfile, p);
                     found_clapf_signature = 1;
                  }
               }
            }
         }
      }

   } while(r);


   free_boundary(state.boundaries);

   return state;
}
#endif

