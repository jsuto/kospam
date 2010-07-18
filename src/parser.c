/*
 * parser.c, SJ
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

   initState(&state);

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
   char *p, *q, puf[MAXBUFSIZE], muf[MAXBUFSIZE], u[SMALLBUFSIZE], token[MAX_TOKEN_LEN], phrase[MAX_TOKEN_LEN], triplet[3*MAX_TOKEN_LEN];
   int i=0, x, b64_len;

   memset(token, 0, MAX_TOKEN_LEN);

   state->line_num++;

   if(*buf == '.' && *(buf+1) == '.') buf++;

   /* undefined message state */
   if(state->is_header == 1 && strchr(buf, ':')) state->message_state = MSG_UNDEF;

   /* skip empty lines */

   if(state->message_rfc822 == 0 && (buf[0] == '\r' || buf[0] == '\n') ){
      state->message_state = MSG_BODY;

      if(state->is_header == 1) state->is_header = 0;

      if(cfg->debug == 1) printf("\n");

      return 0;
   }


   trimBuffer(buf);


   /* skip the first line, if it's a "From <email address> date" format */
   if(state->line_num == 1 && strncmp(buf, "From ", 5) == 0) return 0;


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
      }


      /* we are interested in only From:, To:, Subject:, Received:, Content-*: header lines */
      if(state->message_state <= 0) return 0;
   }




   if((p = strcasestr(buf, "boundary"))){
      x = extract_boundary(p, state);
   }


   /* Content-type: checking */

   if(state->message_state == MSG_CONTENT_TYPE){
      state->message_rfc822 = 0;

      /* extract Content type */

      p = strchr(buf, ':');
      if(p){
         p++;
         if(*p == ' ' || *p == '\t') p++;
         snprintf(state->attachments[state->n_attachments].type, SMALLBUFSIZE-1, "%s", p);
         p = strchr(state->attachments[state->n_attachments].type, ';');
         if(p) *p = '\0';

      }


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

      /* switch (back) to header mode if we encounterd an attachment with
         "message/rfc822" content-type, 2010.05.16, SJ */

      if(strcasestr(buf, "message/rfc822")){
         state->message_rfc822 = 1;
         state->is_header = 1;
      }


      if(strcasestr(buf, "charset") && strcasestr(buf, "UTF-8")) state->utf8 = 1;
   }


   if(state->message_state > 0 && state->message_state <= MSG_SUBJECT && state->message_rfc822 == 1) state->message_rfc822 = 0;


   /* check for textual base64 encoded part, 2005.03.25, SJ */

   if(state->message_state == MSG_CONTENT_TRANSFER_ENCODING){

      if(strcasestr(buf, "base64")){
         state->base64 = 1;
         state->has_base64 = 1;
      }

      if(strcasestr(buf, "quoted-printable")) state->qp = 1;


      if(strcasestr(buf, "image"))
         state->num_of_images++;

      if(strcasestr(buf, "msword"))
         state->num_of_msword++;
   }



   /* skip the boundary itself */

   if(!strstr(buf, "boundary=") && !strstr(buf, "boundary =") && is_boundary(state->boundaries, buf) == 1){
      if(state->has_to_dump == 1){
         close(state->fd);
         if(state->n_attachments < MAX_ATTACHMENTS-1)
            state->n_attachments++;
      }


      state->has_to_dump = 0;

      state->base64 = 0; state->textplain = 0; state->texthtml = 0;
      state->skip_html = 0;
      state->utf8 = 0;
      state->qp = 0;

      //printf("skipping found boundary: %s", buf);
      return 0;      
   }

   /* end of boundary check */


   /* skip non textual stuff */

   if(state->is_header == 0 && state->textplain == 0 && state->texthtml == 0) return 0;


   /* base64 decode buffer */

   if(state->base64 == 1 && state->message_state == MSG_BODY) b64_len = decodeBase64(buf);


   /* fix encoded From:, To: and Subject: lines, 2008.11.24, SJ */

   if(state->message_state == MSG_FROM || state->message_state == MSG_TO || state->message_state == MSG_SUBJECT) fixupEncodedHeaderLine(buf);


   /* fix soft breaks with quoted-printable decoded stuff, 2006.03.01, SJ */

   if(state->qp == 1) fixupSoftBreakInQuotedPritableLine(buf, state);


   /* fix base64 stuff if the line does not end with a line break, 2006.03.01, SJ */

   if(state->base64 == 1) fixupBase64EncodedLine(buf, state);


   if(state->is_header == 0 && state->texthtml == 1) fixupHTML(buf, state, cfg);



   if(state->message_state == MSG_BODY){
      if(state->qp == 1)   decodeQP(buf);
      if(state->utf8 == 1) decodeUTF8(buf);
   }

   decodeURL(buf);

   if(state->texthtml == 1) decodeHTML(buf);



   /* count invalid junk lines and characters */

   state->l_shit += countInvalidJunkLines(buf);
   state->c_shit += countInvalidJunkCharacters(buf, cfg->replace_junk_characters);

   if(state->message_state == MSG_RECEIVED){
      i = 0;
      if(buf[strlen(buf)-2] == ']' && buf[strlen(buf)-1] == ')'){
         i = 1;
      }
   }

   translateLine((unsigned char*)buf, state);

   reassembleToken(buf);

   if(state->is_header == 1) p = strchr(buf, ' ');
   else p = buf;

   if(cfg->debug == 1) printf("%s\n", buf);
   //if(cfg->debug == 1) printf("%d %d/%d/%d %ld/%ld %s\n", state->is_header, state->message_state, state->utf8, state->qp, state->l_shit, state->c_shit, buf);

   do {
      p = split(p, DELIMITER, puf, MAXBUFSIZE-1);

      if(strcasestr(puf, "http://") || strcasestr(puf, "https://")){
         q = puf;
         do {
            q = split_str(q, "http://", u, SMALLBUFSIZE-1);

            if(u[strlen(u)-1] == '.') u[strlen(u)-1] = '\0';

            if(strlen(u) > 2 && strncasecmp(u, "www.w3.org", 10) && strchr(u, '.') ){

               snprintf(muf, MAXBUFSIZE-1, "URL%%%s", u);
               addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);

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

      /* skip email addresses from the To: line, however keep the 'undisclosed-recipient' part */
      if(state->message_state == MSG_TO && strchr(puf, '@') ) continue;



      if(state->message_state == MSG_RECEIVED){
         x = puf[strlen(puf)-1];

         /* 
          * skip Received line token, if
          *    - no punctuation (eg. by, from, esmtp, ...)
          *    - not "unknown"
          *    - it's on the skipped_received_hosts or skipped_received_ips list
          *    - ends with a number and not a valid IP-address (8.14.3, 6.0.3790.211, ...)
          */

         if((!strchr(puf, '.') && strcmp(puf, "unknown")) || ( (x >= 0x30 && x <= 0x39) && (isDottedIPv4Address(puf) == 0 ||  countCharacterInBuffer(puf, '.') != 3) ) ){
            continue;
         }


         /* 
          * fill state.ip and state.hostname fields _after_
          * eliminated all entries matched by skipped_received_ips,
          * and skipped_received_hosts.
          * These entries hold the name and address of the host
          * which hands this email to us.
          */

         if(i == 1 && state->ipcnt <= 1){
            if(isDottedIPv4Address(puf) == 1){
               snprintf(state->ip, SMALLBUFSIZE-1, "%s", puf);
               if(isItemOnList(puf, cfg->skipped_received_ips) == 0) state->ipcnt = 1;
            }
            else {
               snprintf(state->hostname, SMALLBUFSIZE-1, "%s", puf);
            }
         }

         /* 
          * we are interested in the hostname and IP-address of the
          * sending host so skip the rest, eg. email addresses, ...
          */

         continue;
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

      if(state->message_state == MSG_SUBJECT && state->n_subject_token > 2){
         snprintf(triplet, 3*MAX_TOKEN_LEN-1, "%s+%s", phrase, puf);
         addnode(state->token_hash, triplet, DEFAULT_SPAMICITY, 0);
      }

      if(((state->is_header == 1 && state->n_chain_token > 1) || state->n_body_token > 1) && strlen(token) >= MIN_WORD_LEN && state->message_state != MSG_CONTENT_TYPE){
         if(state->message_state == MSG_SUBJECT || state->message_state == MSG_FROM || state->message_state == MSG_TO) snprintf(phrase, MAX_TOKEN_LEN-1, "%s+%s", token, puf);
         else snprintf(phrase, MAX_TOKEN_LEN-1, "%s+%s", token, muf);

         addnode(state->token_hash, phrase, DEFAULT_SPAMICITY, 0);
      }
      snprintf(token, MAX_TOKEN_LEN-1, "%s", muf);

      memset(muf, 0, MAXBUFSIZE);

   } while(p);


   /*
    * prevent the next Received line to overwrite state.ip
    * and state.hostname. Additionally add the sender's
    * hostname and IP-address to the token list
    */

   if(state->ipcnt == 1){
      state->ipcnt = 2;

      if(strlen(state->hostname) > MAX_WORD_LEN) fixFQDN(state->hostname);

      snprintf(muf, MAXBUFSIZE-1, "HEADER*%s", state->hostname);
      addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);
      state->n_token++;

      snprintf(muf, MAXBUFSIZE-1, "HEADER*%s", state->ip);
      addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);
      state->n_token++;

   }


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

   initState(&state);

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

