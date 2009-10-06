/*
 * parser.c, 2009.10.05, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
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
 * initialise parser state
 */

void init_state(struct _state *state){
   int i;

   state->message_state = MSG_UNDEF;

   state->is_header = 1;

   /* by default we are a text/plain message */

   state->textplain = 1;
   state->texthtml = 0;

   state->base64 = 0;
   state->utf8 = 0;

   state->iso_8859_2 = 1;
   state->qp = 0;

   state->base64_lines = 0;
   state->html_comment = 0;

   state->base64_text = 0;

   state->n_token = 0;
   state->n_body_token = 0;
   state->n_chain_token = 0;
   state->n_subject_token = 0;

   state->c_shit = 0;
   state->l_shit = 0;

   state->line_num = 0;

   state->ipcnt = 0;

   state->train_mode = T_TOE;

   memset(state->ctype, 0, MAXBUFSIZE);
   memset(state->ip, 0, SMALLBUFSIZE);
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


/*
 *
 */

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


/*
 * parse buffer
 */

int parse(char *buf, struct _state *state, struct session_data *sdata, struct __config *cfg){
   char *p, *q, *c, huf[MAXBUFSIZE], puf[MAXBUFSIZE], muf[MAXBUFSIZE], tuf[MAXBUFSIZE], u[SMALLBUFSIZE], token[MAX_TOKEN_LEN], phrase[MAX_TOKEN_LEN], ipbuf[IPLEN];
   int x, b64_len;

   memset(token, 0, MAX_TOKEN_LEN);

   state->line_num++;

   /* undefined message state */
   if(state->is_header == 1 && strchr(buf, ':')) state->message_state = MSG_UNDEF;

   /* skip empty lines */

   if(buf[0] == '\r' || buf[0] == '\n'){
      //if(state->base64_lines > 0) state->base64 = 0;
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
         trim(state->from);
      }


      if(state->message_state == MSG_RECEIVED){

      #ifndef HAVE_PROCESS_ALL_RECEIVED_LINES
         if(state->ipcnt < 2){
      #endif
            p = strchr(buf, '[');
            if(p){
               q = strchr(p, ']');
               if(q){

                  /* we care only IPv4 addresses for now, 2005.12.23, SJ */

                  if(q > p+6 && q-p-1 <= IPLEN-1){
                     memset(ipbuf, 0, IPLEN);
                     memcpy(ipbuf, p+1, q-p-1);
                     strncat(ipbuf, ",", IPLEN-1);

                     if(strncmp(ipbuf, "127.", 4) && !strstr(state->ip, ipbuf) && strlen(state->ip) + strlen(ipbuf) < SMALLBUFSIZE-1)
                        strncat(state->ip, ipbuf, SMALLBUFSIZE-1);

                     state->ipcnt++;
                  }
               }
            }
      #ifndef HAVE_PROCESS_ALL_RECEIVED_LINES
         }
      #endif

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

      //if(state->n_attachments < MAX_ATTACHMENTS-1 && state->attachments[state->n_attachments].size > 0)
      //   state->n_attachments++;

      /* extract Content type */

      p = strchr(buf, ':');
      if(p){
         p++;
         if(*p == ' ' || *p == '\t') p++;
         snprintf(state->attachments[state->n_attachments].type, SMALLBUFSIZE-1, "%s", p);
         trim(state->attachments[state->n_attachments].type);
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

         /*snprintf(u, SMALLBUFSIZE-1, "%s-%d", sdata->ttmpfile, state->n_attachments);
         state->has_to_dump = 1;
         state->fd = open(u, O_CREAT|O_RDWR, 0644);*/
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

   if(!strcasestr(buf, "boundary") && is_boundary(state->boundaries, buf) == 1){
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


   /* dump attachment */

   /*if(state->has_to_dump == 1 && state->message_state == MSG_BODY){
      if(state->base64 == 1){
         b64_len = base64_decode(buf, puf);
         write(state->fd, puf, b64_len);
      }
      else write(state->fd, buf, strlen(buf));
   }*/


   /* skip non textual stuff */

   if(state->is_header == 0 && state->textplain == 0 && state->texthtml == 0)
      return 0;

   /* base64 decode buffer, 2005.03.23, SJ */

   if(state->base64 == 1 && state->is_header == 0 && strncmp(buf, "Content-", strlen("Content-")) != 0){
      memset(huf, 0, MAXBUFSIZE);
      b64_len = base64_decode(buf, huf);
      if(b64_len > 0)
         strncpy(buf, huf, MAXBUFSIZE-1);
   }


   /* handle qp encoded lines */

   if(state->qp == 1 || ( (state->message_state == MSG_SUBJECT || state->message_state == MSG_FROM) && strcasestr(buf, "?Q?")) )
      qp_decode(buf);

   /* handle base64 encoded From:, To: and Subject: lines, 2008.11.24, SJ */

   if(state->message_state == MSG_SUBJECT || state->message_state == MSG_FROM || state->message_state == MSG_TO){
      memset(tuf, 0, MAXBUFSIZE);

      q = buf;

      do {
         q = split_str(q, " ", u, SMALLBUFSIZE-1);

         p = strcasestr(u, "?B?");
         if(p){
            *(p+2) = '\0';
            strncat(tuf, u, MAXBUFSIZE-1); strncat(tuf, "? ", MAXBUFSIZE-1);

            *(p+2) = '?';
            base64_decode(p+3, huf);
            strncat(tuf, huf, MAXBUFSIZE-1); strncat(tuf, " ", MAXBUFSIZE-1);
         }
         else {
            strncat(tuf, u, MAXBUFSIZE-1);
            strncat(tuf, " ", MAXBUFSIZE-1);
         }
      } while(q);

      snprintf(buf, MAXBUFSIZE-1, "%s", tuf);
   }


   /* html decode stuff, 2007.06.07, SJ */

   html_decode(buf);


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


   /*
    * utf-8 decoding moved after the quoted-printable fixes, together
    *  with the pre_translate() and url_decode() functions,  2007.05.22, SJ
    */

   if(state->utf8 == 1) utf8_decode(buf);

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


   url_decode(buf);


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
   state->c_shit += count_invalid_junk(buf, cfg->replace_junk_characters);

   /* skip unless we have an URL, 2006.11.09, SJ */

   if(x > 0) state->l_shit += x;

DECOMPOSE:
   /* skip unnecessary header lines */
   if(state->message_state == MSG_UNDEF && state->is_header == 1) return 0;

   translate2((unsigned char*)buf, state->qp);

   reassemble_token(buf);

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

               fix_url(muf);

               addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);
               append_list(&(state->urls), muf);
               state->n_token++;

               /* create a .tld token, suitable for http://whatever.cn/ URIs, 2009.08.05, SJ */

               tld_from_url(muf);

               addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);
               append_list(&(state->urls), muf);
               state->n_token++;
                
            }
         } while(q);

         continue;
      }

      /* if we have a long string in the Received: lines, let's truncate it, 
         and it may be a domain name, 2008.07.22 */

      if(state->message_state == MSG_RECEIVED && strlen(puf) > MAX_WORD_LEN){
         fix_fqdn(puf);

         snprintf(muf, MAXBUFSIZE-1, "%s", puf);
         tld_from_fqdn(muf);
         addnode(state->token_hash, muf, DEFAULT_SPAMICITY, 0);
         state->n_token++;
      }

      /* skip too short or long or numeric only tokens */

      if(strlen(puf) < MIN_WORD_LEN || strlen(puf) > MAX_WORD_LEN || is_hex_number(puf))
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

      /* degenerate token, 2007.05.04, SJ */

      degenerate((unsigned char*)muf);


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


/*
 * parse the message into tokens and return the pointer
 */

struct _state parse_message(char *spamfile, struct session_data *sdata, struct __config *cfg){
   FILE *f;
   int skipped_header = 0, found_clapf_signature = 0;
   char *p, *q;
   char buf[MAXBUFSIZE], tumbuf[SMALLBUFSIZE];
   struct _state state;

   init_state(&state);

   f = fopen(spamfile, "r");
   if(!f){
      syslog(LOG_PRIORITY, "%s: cannot open", spamfile);
      return state;
   }


   snprintf(tumbuf, SMALLBUFSIZE-1, "%sTUM", cfg->clapf_header_field);

   while(fgets(buf, MAXBUFSIZE-1, f)){

      if(sdata->training_request == 0 || found_clapf_signature == 1){
         //syslog(LOG_PRIORITY, "parsing: %s", buf);
         parse(buf, &state, sdata, cfg);
         if(strncmp(buf, tumbuf, strlen(tumbuf)) == 0) state.train_mode = T_TUM;
      }

      if(found_clapf_signature == 0 && sdata->training_request == 1){
         //syslog(LOG_PRIORITY, "skipping: %s", buf);

         if(buf[0] == '\n' || (buf[0] == '\r' && buf[1] == '\n') ){
            skipped_header = 1;
         }

         if(skipped_header == 1){
            q = strstr(buf, "Received: ");
            if(q){
               trim(buf);
               p = strchr(buf, ' ');
               if(p){
                  p++;
                  if(is_valid_id(p)){
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

