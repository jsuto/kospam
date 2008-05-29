/*
 * parser.c, 2008.05.28, SJ
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
#include "parser.h"
#include "shdr.h"
#include "config.h"



/*
 * initialise parser state
 */

void init_state(struct _state *state){
   int i;

   state->message_state = MSG_UNDEF;

   state->is_header = 1;
   state->has_boundary = 0;
   state->has_boundary2 = 0;

   /* by default we are a text/plain message */

   state->textplain = 1;

   state->base64 = 0;
   state->utf8 = 0;

   /* changed to 0->1, 2008.03.07, SJ */
   state->iso_8859_2 = 1;
   state->qp = 0;

   state->html_comment = 0;

   state->base64_text = 0;

   state->n_token = 0;
   state->n_body_token = 0;
   state->n_chain_token = 0;
   state->n_subject_token = 0;

   state->c_hex_shit = 0;
   state->c_shit = 0;
   state->l_shit = 0;

   state->line_num = 0;

   state->ipcnt = 0;

   state->train_mode = T_TOE;

   memset(state->boundary, 0, BOUNDARY_LEN);
   memset(state->boundary2, 0, BOUNDARY_LEN);
   memset(state->ctype, 0, MAXBUFSIZE);
   memset(state->ip, 0, SMALLBUFSIZE);
   memset(state->miscbuf, 0, MAX_TOKEN_LEN);
   memset(state->qpbuf, 0, MAX_TOKEN_LEN);
   memset(state->from, 0, SMALLBUFSIZE);

   state->c_token = NULL;
   state->first = NULL;

   state->urls = NULL;

   state->found_our_signo = 0;

   state->check_attachment = 0;
   state->has_to_dump = 0;
   state->fd = 0;
   state->num_of_images = 0;
   state->num_of_msword = 0;

   state->n_attachments = 0;
   memset(state->attachedfile, 0, SMALLBUFSIZE);

   for(i=0; i<MAX_ATTACHMENTS; i++){
      state->attachments[i].size = 0;
      memset(state->attachments[i].type, 0, SMALLBUFSIZE);
   }

}


/*
 *
 */

int attachment_by_type(struct _state state, char *type){
   int i;

   for(i=0; i<MAX_ATTACHMENTS; i++){
      if(strstr(state.attachments[i].type, type))
         return 1;
   }

   return 0;
}


/*
 * extract bondary
 */

int extract_boundary(char *p, char *boundary, int boundary_len){
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

      strncpy(boundary, p, boundary_len);

      return 1;
   }

   return 0;
}


/*
 * is it a month?
 */

int is_month(char *s){
   int i;
   char *p;

   for(i=0; i<(sizeof(months) / sizeof(p)); i++){
      if(strcasecmp(s, months[i]) == 0)
         return 1;
   }

   return 0;
}


/*
 * is it a weekday?
 */

int is_weekday(char *s){
   int i;
   char *p;

   for(i=0; i<(sizeof(weekdays) / sizeof(p)); i++){
      if(strcasecmp(s, weekdays[i]) == 0)
         return 1;
   }

   return 0;
}


/*
 * is it date related stuff?
 */

int is_date(char *s){
   int i;
   char *p;

   for(i=0; i<(sizeof(dates) / sizeof(p)); i++){
      if(strcasecmp(s, dates[i]) == 0)
         return 1;
   }

   return 0;
}


/*
 * parse buffer
 */

int parse(char *buf, struct _state *state, struct session_data *sdata, struct __config cfg){
   char *p, *q, *c, huf[MAXBUFSIZE], puf[MAXBUFSIZE], muf[MAXBUFSIZE], tuf[MAXBUFSIZE], rnd[RND_STR_LEN], u[SMALLBUFSIZE], token[MAX_TOKEN_LEN], phrase[MAX_TOKEN_LEN], ipbuf[IPLEN];
   int i, x, b64_len;
   int do_utf8, do_base64, do_qp;

   do_utf8 = do_base64 = do_qp = 0;

   state->line_num++;

   /* check for our anti backscatter signo, SJ */

   if(sdata->need_signo_check == 1){
      if(strncmp(buf, cfg.our_signo, strlen(cfg.our_signo)) == 0)
         state->found_our_signo = 1;
   }

   /* header checks */

   if(state->is_header == 1){

      /* end of header? */

      if(buf[0] == '\r' || buf[0] == '\n'){
         state->is_header = 0;
         state->message_state = MSG_BODY;
      }

      /* Subject: */

      if(strncmp(buf, "Subject:", strlen("Subject:")) == 0){
         state->message_state = MSG_SUBJECT;
      }

      /* From: */

      if(strncmp(buf, "From:", strlen("From:")) == 0){
         state->message_state = MSG_FROM;

         p = strchr(buf+5, ' ');
         if(p) p = buf + 6;
         else p = buf + 5;

         snprintf(state->from, SMALLBUFSIZE-1, "FROM*%s", p);
         trim(state->from);
      }

      if(strncmp(buf, "To:", 3) == 0){
         state->message_state = MSG_TO;
      }

      /* Received: 2005.12.09, SJ */

   #ifdef HAVE_PROCESS_ALL_RECEIVED_LINES
      if(strncmp(buf, "Received: from ", strlen("Received: from ")) == 0){
   #else
      if(strncmp(buf, "Received: from ", strlen("Received: from ")) == 0 && state->ipcnt < 2){
   #endif
         state->message_state = MSG_RECEIVED;

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

      }


      /* extract prefix type */

      memset(puf, 0, MAXBUFSIZE);

      if(!isspace(buf[0])){
         p = strchr(buf, ':');
         if(p)
            memcpy(puf, buf, p-buf);
      }

      /* we are interested in only these header lines, 2007.07.10, SJ */

      if(strlen(puf) >= 2 && strcasecmp(puf, "Received") && strcasecmp(puf, "From") && strcasecmp(puf, "To") && strcasecmp(puf, "Subject") &&
           strcasecmp(puf, "Content-Type") && strcasecmp(puf, "Content-Transfer-Encoding") && strcasecmp(puf, "Content-Disposition") )
         return 0;


   }
   else
      state->cnt_type = 0;

   /* end of header checks */



   /* Content-type: checking */

   if(strncasecmp(buf, "Content-Type:", strlen("Content-Type:")) == 0){
      if(state->is_header == 1) state->message_state = MSG_CONTENT_TYPE;

      state->textplain = 0;
      state->base64 = 0;
      state->utf8 = 0;
      state->iso_8859_2 = 0;

      if(state->n_attachments < MAX_ATTACHMENTS-1 && state->attachments[state->n_attachments].size > 0)
         state->n_attachments++;

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

      /* check only the first attachment of this type */

      if( (state->check_attachment == 1 && str_case_str(buf, "image") && state->num_of_images == 0) || (str_case_str(buf, "msword") && state->num_of_msword == 0) ){
         p = strchr(buf, '/');
         q = strchr(buf, ';');
         if(p && q){

            if(str_case_str(buf, "image"))
               state->num_of_images++;

            if(str_case_str(buf, "msword"))
               state->num_of_msword++;

            make_rnd_string(rnd);

            snprintf(state->attachedfile, SMALLBUFSIZE-1, "%s.%s", rnd, p+1);

            // get rid of the semicolon (;)
            q = strchr(state->attachedfile, ';');
            if(q)
               *q = '\0';

            /* writing attachments is disabled at the moment, 2007.09.07, SJ */

            /*state->has_to_dump = 1;
            state->fd = open(state->attachedfile, O_CREAT|O_RDWR, 0644);*/
         }
      }
      else {
         state->has_to_dump = 0;
         if(state->fd)
            close(state->fd);
      }

      /* 2007.04.19, SJ */

      if(str_case_str(buf, "text/plain") ||
         str_case_str(buf, "text/html") ||
         str_case_str(buf, "multipart/mixed") ||
         str_case_str(buf, "multipart/alternative") ||
         str_case_str(buf, "message/delivery-status") ||
         str_case_str(buf, "text/rfc822-headers") ||
         str_case_str(buf, "application/ms-tnef")
      ){

             state->textplain = 1;
      }
      else
         goto DECOMPOSE;
   }


   /* check for textual base64 encoded part, 2005.03.25, SJ */

   if(strncasecmp(buf, "Content-Transfer-Encoding:", strlen("Content-Transfer-Encoding:")) == 0){
      if(state->is_header == 1) state->message_state = MSG_CONTENT_TRANSFER_ENCODING;

      /* check for textual base64 encoded part, 2005.03.25, SJ */
      if(str_case_str(buf, "base64")) state->base64 = 1;
   }

   /* check for UTF-8 encoding */

   if(str_case_str(buf, "charset") && str_case_str(buf, "UTF-8"))
      state->utf8 = 1;

   if(str_case_str(buf, "charset") && (str_case_str(buf, "ISO-8859-2") || str_case_str(buf, "ISO-8859-1"))  ){
      state->iso_8859_2 = 1;
   }

   /* catch encoded stuff in the Subject|From lines, 2007.09.04, SJ */

   if(state->message_state == MSG_SUBJECT || state->message_state == MSG_FROM){
      if(str_case_str(buf, "?iso-8859-2?") || str_case_str(buf, "?iso-8859-1?")) state->iso_8859_2 = 1;
      if(str_case_str(buf, "?utf-8?")) state->utf8 = 1;
   }

   /* check for quoted-printable encoding */

   if(strncasecmp(buf, "Content-Transfer-Encoding:", strlen("Content-Transfer-Encoding:")) == 0 && str_case_str(buf, "quoted-printable"))
      state->qp = 1;

   if(strncasecmp(buf, "Content-Transfer-Encoding:", strlen("Content-Transfer-Encoding:")) == 0 && state->textplain == 0)
      goto DECOMPOSE;

   if(strncasecmp(buf, "Content-Disposition:", strlen("Content-Disposition:")) == 0){
      if(state->is_header == 1) state->message_state = MSG_CONTENT_DISPOSITION;
      if(state->textplain == 0) goto DECOMPOSE;
   }

   /* is it a base64 encoded text? 2006.01.02, SJ */

   if(state->base64_text == 0 && state->textplain == 1 && state->base64 == 1)
      state->base64_text = 1;


   /* boundary checks */

   if(state->has_boundary == 1 && state->cnt_type == 1 && (p = str_case_str(buf, "boundary"))){
      x = extract_boundary(p, state->boundary2, BOUNDARY_LEN-1);
      if(x == 1) state->has_boundary2 = 1;
      state->base64 = 0; state->textplain = 0; // state->qp = 0;
   }

   if(state->cnt_type == 1 && state->has_boundary2 == 0 && (p = str_case_str(buf, "boundary"))){
      x = extract_boundary(p, state->boundary, BOUNDARY_LEN-1);
      if(x == 1) state->has_boundary = 1;

      /* do not let the boundary definition reset the Content-* variables if we are in the header, 2006.03.13, SJ */

      if(state->is_header == 1 && strncasecmp(buf, "Content-Type:", strlen("Content-Type:")) == 0){
         state->base64 = 0; // state->qp = 0;
      }
      else {
         state->base64 = 0; state->textplain = 0; // state->qp = 0;
      }

   }

   /* skip the boundary itself */

   if(state->has_boundary == 1 && !str_case_str(buf, "boundary") && strstr(buf, state->boundary)){
      return 0;
   }

   if(state->has_boundary2 == 1 && !str_case_str(buf, "boundary") && strstr(buf, state->boundary2))
      return 0;

   /* end of boundary check */


   /* dump attachment, 2007.07.03, SJ */

   if(!strchr(buf, '\t') && buf[0] != '\n' && buf[0] != '\r' && state->fd != -1){
      if(state->base64 == 1 && !strchr(buf, ' ')){
         b64_len = base64_decode(buf, puf);
         state->attachments[state->n_attachments].size += b64_len;
         if(state->has_to_dump == 1)
            write(state->fd, puf, b64_len);
      }
      else if(state->is_header == 0){
         state->attachments[state->n_attachments].size += strlen(buf);
      }
   }

   /* skip non textual stuff */

   if(state->is_header == 0 && state->textplain == 0)
      return 0;

   /* base64 decode buffer, 2005.03.23, SJ */

   if(state->base64 == 1 && state->is_header == 0 && strncmp(buf, "Content-", strlen("Content-")) != 0){
      memset(huf, 0, MAXBUFSIZE);
      b64_len = base64_decode(buf, huf);
      if(b64_len > 0)
         strncpy(buf, huf, MAXBUFSIZE-1);
   }


   /* handle qp encoded lines */

   if(state->qp == 1 || ( (state->message_state == MSG_SUBJECT || state->message_state == MSG_FROM) && str_case_str(buf, "?Q?")) )
      qp_decode((unsigned char*)buf);


   /* handle base64 encoded subject */

   if( (state->message_state == MSG_SUBJECT || state->message_state == MSG_FROM) && (p = str_case_str(buf, "?B?"))){
      base64_decode(p+3, huf);
      *(p+3) = '\0';
      snprintf(tuf, MAXBUFSIZE-1, "%s%s", buf, huf);
      snprintf(buf, MAXBUFSIZE-1, "%s", tuf);
   }


   /* html decode stuff, 2007.06.07, SJ */

   html_decode(buf);


   /* 
    * skip message headers we don't want. Look shdr.h and modify/update the header lines
    * you want to skip, 2006.02.20, SJ
    */

   for(i=0; i<(sizeof(skip_headers) / sizeof(p)); i++){
      if(strncasecmp(buf, skip_headers[i], strlen(skip_headers[i])) == 0)
         return 0;
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


   /*
    * utf-8 decoding moved after the quoted-printable fixes, together
    *  with the pre_translate() and url_decode() functions,  2007.05.22, SJ
    */

   if(state->utf8 == 1) utf8_decode((unsigned char*)buf);

   /* handle html comments, 2007.06.07, SJ */

   if(state->html_comment == 1 && strchr(buf, '>')) state->html_comment = 0;

   if(state->is_header == 0 && strstr(buf, "<!")) state->html_comment = 1;

   if(state->html_comment == 1){
      q = strstr(buf, "<!");
      if(q){
         *q = '\0';

      #ifdef DEBUG
         fprintf(stderr, "DISCARDED HTML: %s", ++q);
      #endif
      }
      //return 0;
   }



   pre_translate(buf);
   url_decode(buf);

   /*
    * try to detect Chinese, Korean, .... (sorry guys) text, like  ^[$B.......^[(B
    * I think these should not occur in ordinary European languages, 2006.02.02, SJ
    */

   x = 0;

   if(buf[0] == '' && buf[1] == '$' && buf[2] == 'B'){
      c = buf;
      for(; *c; c++){
         if(*c == '' && *(c+1) == '(' && *(c+2) == 'B')
            x++;
      }
   }

   /* count the number of hexa and junk characters, 2006.11.09, SJ */

   state->c_hex_shit += count_invalid_hexa_stuff((unsigned char*)buf);

   /* count invalid junk characters unless UTF-8 encoded or ISO-8859-2 part, 2007.04.04, SJ */
   if(state->utf8 == 0 && state->iso_8859_2 == 0) state->c_shit += count_invalid_junk((unsigned char*)buf);

   /* skip unless we have an URL, 2006.11.09, SJ */

   if(x > 0){
      state->l_shit += x;
      if(!str_case_str(buf, "http://") && !str_case_str(buf, "https://"))
         return 0;
   }

   /* translate junk characters to JUNK_REPLACEMENT_CHAR, 2007.09.04, SJ */

   if(state->utf8 == 0 && state->iso_8859_2 == 0){
      for(i=0; i<strlen(buf); i++)
         if(buf[i] < 0) buf[i] = JUNK_REPLACEMENT_CHAR;
   }

DECOMPOSE:
   translate((unsigned char*)buf, state->qp);

   if(state->is_header == 1) p = strchr(buf, ' ');
   else p = buf;

#ifdef DEBUG
   //fprintf(stderr, "%ld * %s\n", state->c_shit, p);
   fprintf(stderr, "%s\n", buf);
#endif

   do {
      p = split(p, DELIMITER, puf, MAXBUFSIZE-1);

      /* handle URLs, 2006.12.11, SJ */

      if(strncasecmp(puf, "http://", 7) == 0 || strncasecmp(puf, "https://", 8) == 0){
         q = puf;
         do {
            q = split_str(q, "http://", u, SMALLBUFSIZE-1);
            if(strlen(u) > 2){
               snprintf(muf, MAXBUFSIZE-1, "http://%s", u);

               fix_url(muf);

               insert_token(state, muf);

               append_url(state, muf);

               state->n_token++;
            }
         } while(q);

         continue;
      }


      /* skip too short or long or numeric only tokens */
      if(strlen(puf) < MIN_WORD_LEN || strlen(puf) > MAX_WORD_LEN || is_hex_number(puf))
         continue;

      /* skip date tokens */
      if(is_odd_punctuations(puf) == 1 || is_month(puf) == 1 || is_weekday(puf) == 1 || is_date(puf) )
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

      insert_token(state, muf);


      /* create token pairs, 2007.06.06, SJ */

      if(state->is_header == 0) state->n_body_token++;

      if(((state->is_header == 1 && state->n_chain_token > 1) || state->n_body_token > 1) && strlen(token) >= MIN_WORD_LEN && state->message_state != MSG_CONTENT_TYPE){
         snprintf(phrase, MAX_TOKEN_LEN-1, "%s+%s", token, muf);
         insert_token(state, phrase);
      }
      snprintf(token, MAX_TOKEN_LEN-1, "%s", muf);

      memset(muf, 0, MAXBUFSIZE);

   } while(p);


   /* do not chain between individual headers, 2007.06.09, SJ */
   if(state->is_header == 1) state->n_chain_token = 0;

   if(state->message_state == MSG_FROM && strlen(state->from) > 3){
      insert_token(state, state->from);
   }

   return 0;
}

