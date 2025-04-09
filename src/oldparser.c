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
#include <clapf.h>


struct __state parse_message2(struct session_data *sdata, int take_into_pieces, struct __config *cfg){
   FILE *f;
   int tumlen;
   int skipped_header = 0, found_clapf_signature = 0;
   char buf[MAXBUFSIZE];
   char tumbuf[SMALLBUFSIZE];
   char abuffer[MAXBUFSIZE];
   char *p;
   struct __state state;

   init_state(&state);

   f = fopen(sdata->filename, "r");
   if(!f){
      syslog(LOG_PRIORITY, "%s: error: cannot open", sdata->ttmpfile);
      return state;
   }

   snprintf(tumbuf, sizeof(tumbuf)-1, "%sTUM", cfg->clapf_header_field);
   tumlen = strlen(tumbuf);

   size_t spamicity_header_len = strlen(cfg->clapf_header_field);

   while(fgets(buf, sizeof(buf)-1, f)){

      if(sdata->training_request == 0 || found_clapf_signature == 1){
         parse_line(buf, &state, sdata, take_into_pieces, &abuffer[0], sizeof(abuffer), cfg);
         if(strncmp(buf, tumbuf, tumlen) == 0) state.train_mode = T_TUM;
      }


      if(found_clapf_signature == 0 && sdata->training_request == 1){

         if(buf[0] == '\n' || (buf[0] == '\r' && buf[1] == '\n') ){
            skipped_header = 1;
         }

         if(skipped_header == 1){

            // FIXME: get the legacy clapf id from the X-Clapf-spamicity: header

            //if(strncmp(buf, "Received: ", 10) == 0){
            if(strncmp(buf, cfg->clapf_header_field, spamicity_header_len) == 0){
               trim_buffer(buf);
               p = strchr(buf, ' ');
               if(p){
                  p++;

                  while(*p == ' ') p++;

                  if(is_valid_clapf_id(p)){
                     snprintf(sdata->clapf_id, SMALLBUFSIZE-1, "%s", p);
                     printf("%s: found id in training request: *%s*\n", sdata->ttmpfile, p);

                     if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: found id in training request: *%s*", sdata->ttmpfile, p);
                     found_clapf_signature = 1;
                  }
               }
            }
         }

      }


   }

   fclose(f);

   return state;
}


void post_parse2(struct __state *state){
   int i;

   trim_buffer(state->b_subject);

   state->message_state = MSG_SUBJECT;
   translate_line((unsigned char*)state->b_subject, state);

   //generate_tokens_from_string(state, state->b_from, "HEADER*");
   //generate_tokens_from_string(state, state->b_from_domain, "HEADER*");
   //state->n_subject_token = generate_tokens_from_string(state, state->b_subject, "SUBJ*");
   //state->n_token = generate_tokens_from_string(state, state->b_body, "");
   addnode(state->token_hash, state->from, DEFAULT_SPAMICITY, 0);

   clearhash(state->boundaries);

   for(i=1; i<=state->n_attachments; i++){
      digest_file(state->attachments[i].internalname, &(state->attachments[i].digest[0]));

      unlink(state->attachments[i].internalname);


      /* can we skip this? */

      if(state->attachments[i].dumped == 1){
         unlink(state->attachments[i].aname);
      }

   }


   if(state->message_id[0] == 0){
      addnode(state->token_hash, "NO_MESSAGE_ID*",  REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
   }

}


void storno_attachment(struct __state *state){
   state->has_to_dump = 0;

   if(state->n_attachments <= 0) return;

   state->attachments[state->n_attachments].size = 0;
   state->attachments[state->n_attachments].dumped = 0;

   memset(state->attachments[state->n_attachments].type, 0, TINYBUFSIZE);
   memset(state->attachments[state->n_attachments].shorttype, 0, TINYBUFSIZE);
   memset(state->attachments[state->n_attachments].aname, 0, TINYBUFSIZE);
   memset(state->attachments[state->n_attachments].filename, 0, TINYBUFSIZE);
   memset(state->attachments[state->n_attachments].internalname, 0, TINYBUFSIZE);
   memset(state->attachments[state->n_attachments].digest, 0, 2*DIGEST_LENGTH+1);


   state->n_attachments--;
}


int parse_line(char *buf, struct __state *state, struct session_data *sdata, int take_into_pieces, char *abuffer, int abuffersize, struct __config *cfg){
   char *p;
   unsigned char b64buffer[MAXBUFSIZE];
   char tmpbuf[MAXBUFSIZE];
   int n64, len, boundary_line=0, result;

   //if(cfg->debug == 1) printf("line: %s", buf);

   state->line_num++;
   len = strlen(buf);

   if(state->message_rfc822 == 0 && (buf[0] == '\r' || buf[0] == '\n') ){
      state->message_state = MSG_BODY;

      if(state->is_header == 1) state->is_header = 0;
      state->is_1st_header = 0;

      if(state->anamepos > 0){
         extract_name_from_header_line(state->attachment_name_buf, "name", state->filename, sizeof(state->filename)-1);
      }

   }


   if(state->message_state == MSG_BODY && state->attachment == 1 && is_substr_in_hash(state->boundaries, buf) == 0){
      if(take_into_pieces == 1){
         if(state->fd != -1 && len + state->abufpos > abuffersize-1){
            if(write(state->fd, abuffer, state->abufpos) == -1) syslog(LOG_PRIORITY, "ERROR: %s: write(), %s, %d, %s", sdata->ttmpfile, __func__, __LINE__, __FILE__);

            if(state->b64fd != -1){
               abuffer[state->abufpos] = '\0';
               if(state->base64 == 1){
                  n64 = base64_decode_attachment_buffer(abuffer, &b64buffer[0], sizeof(b64buffer));
                  n64 = write(state->b64fd, b64buffer, n64);
               }
               else {
                  n64 = write(state->b64fd, abuffer, state->abufpos);
               }
            }

            state->abufpos = 0; memset(abuffer, 0, abuffersize);
         }
         memcpy(abuffer+state->abufpos, buf, len); state->abufpos += len;
      }

      state->attachments[state->n_attachments].size += len;
   }




   if(state->message_state == MSG_BODY && state->has_to_dump == 1 &&  state->pushed_pointer == 0){

      state->pushed_pointer = 1;

      // this is a real attachment to dump, it doesn't have to be base64 encoded!
      if(strlen(state->filename) > 4 && strlen(state->type) > 3 && state->n_attachments < MAX_ATTACHMENTS-1){
         state->n_attachments++;

         snprintf(state->attachments[state->n_attachments].filename, TINYBUFSIZE-1, "%s", state->filename);
         snprintf(state->attachments[state->n_attachments].type, TINYBUFSIZE-1, "%s", state->type);
         snprintf(state->attachments[state->n_attachments].internalname, TINYBUFSIZE-1, "%s.a%d", sdata->ttmpfile, state->n_attachments);
         snprintf(state->attachments[state->n_attachments].aname, TINYBUFSIZE-1, "%s.a%d.bin", sdata->ttmpfile, state->n_attachments);

         //printf("DUMP FILE: %s\n", state->attachments[state->n_attachments].internalname);

         state->attachment = 1;

         fixupEncodedHeaderLine(state->attachments[state->n_attachments].filename, TINYBUFSIZE);
         p = get_attachment_extractor_by_filename(state->attachments[state->n_attachments].filename);
         snprintf(state->attachments[state->n_attachments].shorttype, TINYBUFSIZE-1, "%s", p);

         if(take_into_pieces == 1){
            state->fd = open(state->attachments[state->n_attachments].internalname, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);

            if(strcmp("other", p)){
               state->b64fd = open(state->attachments[state->n_attachments].aname, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
               state->attachments[state->n_attachments].dumped = 1;
            }

            if(state->fd == -1){
               storno_attachment(state);
               syslog(LOG_PRIORITY, "%s: error opening %s", sdata->ttmpfile, state->attachments[state->n_attachments].internalname);
            }
         }

      }
      // don't dump, not an attached file
      else {
         state->has_to_dump = 0;
      }

   }



   if(*buf == '.' && *(buf+1) == '.') buf++;

   /* undefined message state */
   if(state->is_header == 1 && buf[0] != ' ' && buf[0] != '\t' && strchr(buf, ':')) state->message_state = MSG_UNDEF;

   /* skip empty lines */

   if(state->message_rfc822 == 0 && (buf[0] == '\r' || buf[0] == '\n') ){
      return 0;
   }


   trim_buffer(buf);

   /* check for our anti backscatter signo, SJ */

   if(sdata->need_signo_check == 1){
      if(strncmp(buf, cfg->our_signo, strlen(cfg->our_signo)) == 0)
         state->found_our_signo = 1;
   }


   /* skip the first line, if it's a "From <email address> date" format */
   if(state->line_num == 1 && strncmp(buf, "From ", 5) == 0) return 0;

   if(state->is_header == 0 && buf[0] != ' ' && buf[0] != '\t') state->message_state = MSG_BODY;


   /* header checks */

   if(state->is_header == 1){

      if(strncasecmp(buf, "From:", strlen("From:")) == 0){
         state->message_state = MSG_FROM;

         if(state->is_1st_header == 1){
            p = buf+5;
            if(*p == ' ') p++;
            snprintf(state->from, SMALLBUFSIZE-1, "FROM*%s", p);

            if(is_list_on_string(p, cfg->mydomains) == 1) sdata->from_address_in_mydomain = 1;
         }
      }

      else if(strncasecmp(buf, "Content-Type:", strlen("Content-Type:")) == 0){
         state->message_state = MSG_CONTENT_TYPE;

         if(state->anamepos > 0){
            extract_name_from_header_line(state->attachment_name_buf, "name", state->filename, sizeof(state->filename)-1);
            memset(state->attachment_name_buf, 0, SMALLBUFSIZE);
            state->anamepos = 0;
         }

      }
      else if(strncasecmp(buf, "Content-Transfer-Encoding:", strlen("Content-Transfer-Encoding:")) == 0) state->message_state = MSG_CONTENT_TRANSFER_ENCODING;
      else if(strncasecmp(buf, "Content-Disposition:", strlen("Content-Disposition:")) == 0){
         state->message_state = MSG_CONTENT_DISPOSITION;

         if(state->anamepos > 0){
            extract_name_from_header_line(state->attachment_name_buf, "name", state->filename, sizeof(state->filename)-1);
            memset(state->attachment_name_buf, 0, SMALLBUFSIZE);
            state->anamepos = 0;
         }

      }
      else if(strncasecmp(buf, "To:", 3) == 0) state->message_state = MSG_TO;
      else if(strncasecmp(buf, "Cc:", 3) == 0) state->message_state = MSG_CC;
      else if(strncasecmp(buf, "Bcc:", 4) == 0) state->message_state = MSG_CC;
      else if(strncasecmp(buf, "Message-Id:", 11) == 0) state->message_state = MSG_MESSAGE_ID;
      else if(strncasecmp(buf, "References:", 11) == 0) state->message_state = MSG_REFERENCES;
      else if(strncasecmp(buf, "Subject:", strlen("Subject:")) == 0) state->message_state = MSG_SUBJECT;
      else if(strncasecmp(buf, "Recipient:", strlen("Recipient:")) == 0) state->message_state = MSG_RECIPIENT;
      else if(strncasecmp(buf, "Received:", strlen("Received:")) == 0) state->message_state = MSG_RECEIVED;
      else if(state->line_num == 1 && strncasecmp(buf, "Kospam-Envelope-From: ", strlen("Kospam-Envelope-From: ")) == 0) {
         snprintf(sdata->fromemail, sizeof(sdata->fromemail)-1, "%s", buf+strlen("Kospam-Envelope-From: "));
         trim_buffer(sdata->fromemail);
      } else if(state->line_num == 2 && strncasecmp(buf, "Kospam-Envelope-Recipient: ", strlen("Kospam-Envelope-Recipient: ")) == 0) {
         char *p = buf + strlen("Kospam-Envelope-Recipient: ");

         int i = 0;
         while (p) {
            int result;
            if (i < MAX_RCPT_TO) {
               p = split(p, ',', sdata->rcptto[i], SMALLBUFSIZE-1, &result);
               printf("rcpt %d: *%s*\n", i, sdata->rcptto[i]);
               if (i == 0 && ( strstr(sdata->rcptto[i], "spam@") ||
                               strstr(sdata->rcptto[i], "+spam@") ||
                               strstr(sdata->rcptto[i], "ham@") ||
                               strstr(sdata->rcptto[i], "+ham@") )
               ) {
                   sdata->training_request = 1;
               }

               sdata->num_of_rcpt_to++;
            } else {
               break;
            }

            i++;
         }
      } else if(state->line_num == 3 && strncasecmp(buf, "Kospam-Xforward: ", strlen("Kospam-Xforward: ")) == 0) {
         char *p = buf + strlen("Kospam-Xforward: ");

         // Data is expected in this order
         // NAME=smtp.example.com ADDR=1.2.3.4 PROTO=ESMTP HELO=smtp.example.com

         int i = 0;
         while (p) {
            int result;
            char v[SMALLBUFSIZE];
            p = split(p, ',', v, sizeof(v)-1, &result);

            if(strlen(v) > 5) sdata->ipcnt++;

            if (i == 0) {
               snprintf(state->hostname, sizeof(state->hostname)-1, "%s", v);
            }
            if (i == 1) {
               snprintf(state->ip, sizeof(state->ip)-1, "%s", v);
            }

            i++;
         }
      }

      if(state->message_state == MSG_MESSAGE_ID && state->message_id[0] == 0){
         p = strchr(buf+11, ' ');
         if(p) p = buf + 12;
         else p = buf + 11;

         snprintf(state->message_id, SMALLBUFSIZE-1, "%s", p);
      }

      /* we are interested in only From:, To:, Subject:, Received:, Content-*: header lines */
      if(state->message_state <= 0) return 0;
   }


   if(state->message_state == MSG_CONTENT_TYPE){
      if((p = strcasestr(buf, "boundary"))){
         extract_boundary(p, state);
      }
   }


   if(state->message_state == MSG_RECIPIENT){
      p = strstr(buf, "Expanded:");
      if(p) *p = '\0';
   }


   if(state->is_1st_header == 1){

      if(state->message_state == MSG_SUBJECT && strlen(state->b_subject) + strlen(buf) < MAXBUFSIZE-1){

         if(state->b_subject[0] == '\0'){
            p = &buf[0];
            if(strncmp(buf, "Subject:", strlen("Subject:")) == 0) p += strlen("Subject:");
            if(*p == ' ') p++;

            fixupEncodedHeaderLine(p, MAXBUFSIZE);
            strncat(state->b_subject, p, MAXBUFSIZE-strlen(state->b_subject)-1);
         }
         else {

            /*
             * if the next subject line is encoded, then strip the whitespace characters at the beginning of the line
             */

            p = buf;

            if(strcasestr(buf, "?Q?") || strcasestr(buf, "?B?")){
               while(isspace(*p)) p++;
            }

            fixupEncodedHeaderLine(p, MAXBUFSIZE);

            strncat(state->b_subject, p, MAXBUFSIZE-strlen(state->b_subject)-1);
         }
      }
      else { fixupEncodedHeaderLine(buf, MAXBUFSIZE); }
   }


   /* Content-type: checking */

   if(state->message_state == MSG_CONTENT_TYPE){
      state->message_rfc822 = 0;

      /* extract Content type */

      p = strchr(buf, ':');
      if(p){
         p++;
         if(*p == ' ' || *p == '\t') p++;
         snprintf(state->type, TINYBUFSIZE-1, "%s", p);
         p = strchr(state->type, ';');
         if(p) *p = '\0';
      }


      if(strcasestr(buf, "text/plain") ||
         strcasestr(buf, "multipart/mixed") ||
         strcasestr(buf, "multipart/alternative") ||
         strcasestr(buf, "multipart/report") ||
         strcasestr(buf, "message/delivery-status") ||
         strcasestr(buf, "text/rfc822-headers") ||
         strcasestr(buf, "message/rfc822")
      ){
         state->textplain = 1;
      }
      else if(strcasestr(buf, "text/html")){
         state->texthtml = 1;
      }

      /* switch (back) to header mode if we encounterd an attachment with "message/rfc822" content-type */

      if(strcasestr(buf, "message/rfc822")){
         state->message_rfc822 = 1;
         state->is_header = 1;
      }


      if(strcasestr(buf, "charset")) extract_name_from_header_line(buf, "charset", state->charset, sizeof(state->charset)-1);
      if(strcasestr(state->charset, "UTF-8")) state->utf8 = 1;
   }


   if((state->message_state == MSG_CONTENT_TYPE || state->message_state == MSG_CONTENT_DISPOSITION) && strlen(state->filename) < 5){

      p = &buf[0];
      for(; *p; p++){
         if(*p != ' ' && *p != '\t') break;
      }

      len = strlen(p);

      if(len + state->anamepos < SMALLBUFSIZE-2){
         memcpy(&(state->attachment_name_buf[state->anamepos]), p, len);
         state->anamepos += len;
      }
   }


   if(state->message_state == MSG_CONTENT_TRANSFER_ENCODING){
      if(strcasestr(buf, "base64")) state->base64 = 1;
      if(strcasestr(buf, "quoted-printable")) state->qp = 1;
   }



   /* boundary check, and reset variables */

   boundary_line = is_substr_in_hash(state->boundaries, buf);


   if(!strstr(buf, "boundary=") && !strstr(buf, "boundary =") && boundary_line == 1){
      state->is_header = 1;

      if(state->has_to_dump == 1){
         if(take_into_pieces == 1 && state->fd != -1){
            if(state->abufpos > 0){
               if(write(state->fd, abuffer, state->abufpos) == -1) syslog(LOG_PRIORITY, "ERROR: %s: write(), %s, %d, %s", sdata->ttmpfile, __func__, __LINE__, __FILE__);

               if(state->b64fd != -1){
                  abuffer[state->abufpos] = '\0';
                  if(state->base64 == 1){
                     n64 = base64_decode_attachment_buffer(abuffer, &b64buffer[0], sizeof(b64buffer));
                     n64 = write(state->b64fd, b64buffer, n64);
                  }
                  else {
                     n64 = write(state->b64fd, abuffer, state->abufpos);
                  }
               }

               state->abufpos = 0; memset(abuffer, 0, abuffersize);
            }
            close(state->fd);
            close(state->b64fd);
         }
         state->fd = -1;
         state->b64fd = -1;
         state->attachment = -1;
      }


      state->has_to_dump = 1;

      state->base64 = 0; state->textplain = 0; state->texthtml = 0;
      state->skip_html = 0;
      state->utf8 = 0;
      state->qp = 0;

      state->pushed_pointer = 0;

      memset(state->filename, 0, TINYBUFSIZE);
      memset(state->type, 0, TINYBUFSIZE);
      snprintf(state->charset, TINYBUFSIZE-1, "unknown");

      memset(state->attachment_name_buf, 0, SMALLBUFSIZE);
      state->anamepos = 0;

      state->message_state = MSG_UNDEF;

      return 0;
   }

   if(boundary_line == 1){ return 0; }


   /* end of boundary check */


   /* skip irrelevant headers */
   if(state->is_header == 1 && state->message_state != MSG_FROM && state->message_state != MSG_TO && state->message_state != MSG_CC && state->message_state != MSG_RECIPIENT && state->message_state != MSG_RECEIVED) return 0;


   /* don't process body if it's not a text or html part */
   if(state->message_state == MSG_BODY && state->textplain == 0 && state->texthtml == 0) return 0;


   if(state->base64 == 1 && state->message_state == MSG_BODY){
      decodeBase64(buf);
      fixupBase64EncodedLine(buf, state);
   }

   if(state->message_state == MSG_BODY && state->qp == 1){
      fixupSoftBreakInQuotedPritableLine(buf, state);
      decodeQP(buf);
   }

   if(state->texthtml == 1 && state->message_state == MSG_BODY) remove_html(buf, state);

   /* I believe that we can live without this function call */
   //decodeURL(buf);

   if(state->texthtml == 1) decodeHTML(buf, state->utf8);

   /* encode the body if it's not utf-8 encoded */
   if(state->message_state == MSG_BODY && state->utf8 != 1){
      result = utf8_encode(buf, strlen(buf), &tmpbuf[0], sizeof(tmpbuf), state->charset);
      if(result == OK) snprintf(buf, MAXBUFSIZE-1, "%s", tmpbuf);
   }


   /* count invalid junk lines and characters */
   /*
    * FIXME: utf8 tokens _have_ these characters, so
    * the ijc.h file may not be much help
    */
   //state->l_shit += count_invalid_junk_lines(buf);
   //state->c_shit += count_invalid_junk_characters(buf, 0);


   tokenize(buf, state, sdata, cfg);

   return 0;
}
