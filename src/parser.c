#include <kospam.h>

#define APPENDTOBODY(p, data, pos) do { \
    size_t __len = strlen(p); \
    if (__len + pos + 1 < sizeof(data)) { \
        memcpy(data + pos, (p), __len); \
        pos += __len; \
    } \
} while (0)

#define CONVERT_WHITESPACE_TO_UNDERSCORE(p) do { \
    char *s = p; \
    for(; *s; s++) { if(isspace(*s) || *s == '(' || *s == ')') *s = '_'; } \
} while (0)


void init_state(struct parser_state *state){
   memset((char*)state, 0, sizeof(*state)); // sizeof(state) is only 8 bytes!

   state->tre = '-';
   state->trapped = false;

   inithash(state->token_hash);
}

int parse_message(const char *message, struct parser_state *state, struct Message *m, struct config *cfg) {
    size_t s;
    char *buffer = read_file(message, &s);

    if (!buffer) {
        syslog(LOG_PRIORITY, "ERROR: failed to read %s", message);
        return 1;
    }

    memset((char *)m, 0, sizeof(*m));

    parse_eml_buffer(buffer, m, cfg);

    free(buffer);

    init_state(state);

    utf8_tolower(m->body.data);
    normalize_buffer(m->body.data);

    state->has_image_attachment = m->has_image_attachment;
    state->has_octet_stream_attachment = m->has_octet_stream_attachment;
    state->need_signo_check = m->need_signo_check;
    state->found_our_signo = m->found_our_signo;

    return 0;
}

int post_parse(struct parser_state *state, struct Message *m, struct config *cfg) {

    state->n_token = generate_tokens_from_string(state, m->body.data, "", cfg);

    if (m->header.subject[0]) {
        state->n_subject_token = generate_tokens_from_string(state, m->header.subject, "SUBJ*", cfg);
        state->n_token += generate_tokens_from_string(state, m->header.subject, "", cfg);
        snprintf(state->b_subject, sizeof(state->b_subject)-1, "%s", m->header.subject);
    }

    if (m->header.kospam_watermark[0]) {
        snprintf(state->kospam_watermark, sizeof(state->kospam_watermark)-1, "%s", m->header.kospam_watermark);
    }

    if (m->header.from[0]) {
        generate_tokens_from_string(state, m->header.from, "HEADER*", cfg);

        char tmp[SMALLBUFSIZE];
        snprintf(tmp, sizeof(tmp)-1, "FROM*%s", m->header.from);
        addnode(state->token_hash, tmp, DEFAULT_SPAMICITY, 0);

        // Add the From: domain as a token
        char *p = strchr(m->header.from, '@');
        if (p) generate_tokens_from_string(state, p, "HEADER*", cfg);
    }

    if (m->header.message_id[0] == 0) {
        addnode(state->token_hash, "NO_MESSAGE_ID*",  REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
    }

    if (m->kospam.kospam_envelope_from[0]) {
        snprintf(state->envelope_from, sizeof(state->envelope_from)-1, "%s", m->kospam.kospam_envelope_from);
    }

    if (m->kospam.kospam_envelope_recipient[0]) {
        char *p = m->kospam.kospam_envelope_recipient;

        int i = 0;
        while (p) {
            int result;
            char v[SMALLBUFSIZE];
            p = split(p, ',', v, sizeof(v)-1, &result);

            if (i == 0) {
                snprintf(state->envelope_recipient, sizeof(state->envelope_recipient)-1, "%s", v);
            }

            if (strstr(v, "spam@") ||
                strstr(v, "+spam@") ||
                strstr(v, "ham@") ||
                strstr(v, "+ham@")
            ) {
                state->training_request = 1;
            }

            if(cfg->blackhole_email_list[0]) {
                if (strcasestr(cfg->blackhole_email_list, v)) {
                    state->trapped = true;
                }
            }

            i++;
        }
    }

    if (m->kospam.kospam_xforward[0]) {
        // Data is expected in this order
        // NAME=smtp.example.com ADDR=1.2.3.4 PROTO=ESMTP HELO=smtp.example.com

        int i = 0;
        char *p = m->kospam.kospam_xforward;

        while (p) {
            int result;
            char v[SMALLBUFSIZE];
            p = split(p, ',', v, sizeof(v)-1, &result);

            if (i == 0) {
               snprintf(state->hostname, sizeof(state->hostname)-1, "%s", v);
            }
            if (i == 1) {
               snprintf(state->ip, sizeof(state->ip)-1, "%s", v);
            }

            i++;
        }
    }

    if (!m->kospam.kospam_xforward[0] && m->header.received[0]) {
        char *p = strstr(m->header.received, " by ");
        if (p) *p = '\0';

        // from mail.trops.eu (mail.trops.eu [37.220.140.203])
        p = m->header.received;

        while (p) {
            int result;
            char v[SMALLBUFSIZE];
            p = split(p, ' ' , v, sizeof(v)-1, &result);
            if (strchr(v, '.')) {
                char *q = &v[0];
                if (*q == '(') q++;
                char *r = strrchr(q, ')');
                if (r) *r = '\0';
                if (*q == '[') q++;
                size_t len = strlen(q);
                if (*(q+len-1) == ']') *(q+len-1) = '\0';
                if(is_dotted_ipv4_address(q) == 1){
                    if(is_item_on_list(q, cfg->skipped_received_ips) == false && is_item_on_list(q, "127.,10.,192.168.,172.16.") == false) {
                       snprintf(state->ip, sizeof(state->ip)-1, "%s", q);
                    }
                }
                else {
                    snprintf(state->hostname, sizeof(state->hostname)-1, "%s", q);
                }
            } else if(!strcmp(v, "unknown")) {
                addnode(state->token_hash, "UNKNOWN_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            }
        }
    }

    return 0;
}


int parse_eml_buffer(char *buffer, struct Message *m, struct config *cfg) {
    int body_offset = 4;

    char *headers_end = strstr(buffer, "\r\n\r\n");
    if (!headers_end) {
       headers_end = strstr(buffer, "\n\n");
       body_offset = 2;
    }

    int buffer_len;
    if (headers_end) {
       *headers_end = '\0';
       buffer_len = headers_end - buffer;
    } else {
       // TODO: is it a header-only message?
       buffer_len = strlen(buffer);
    }

    // Extract headers

    extract_header_value(buffer, buffer_len, HEADER_FROM, strlen(HEADER_FROM), m->header.from, sizeof(m->header.from));
    extract_header_value(buffer, buffer_len, HEADER_SUBJECT, strlen(HEADER_SUBJECT), m->header.subject, sizeof(m->header.subject));
    extract_header_value(buffer, buffer_len, HEADER_MESSAGE_ID, strlen(HEADER_MESSAGE_ID), m->header.message_id, sizeof(m->header.message_id));
    extract_header_value(buffer, buffer_len, HEADER_CONTENT_TYPE, strlen(HEADER_CONTENT_TYPE), m->header.content_type, sizeof(m->header.content_type));
    extract_header_value(buffer, buffer_len, HEADER_CONTENT_TRANSFER_ENCODING, strlen(HEADER_CONTENT_TRANSFER_ENCODING), m->header.content_encoding, sizeof(m->header.content_encoding));

    // Kospam-* headers
    extract_header_value(buffer, buffer_len, HEADER_KOSPAM_ENVELOPE_FROM, strlen(HEADER_KOSPAM_ENVELOPE_FROM), m->kospam.kospam_envelope_from, sizeof(m->kospam.kospam_envelope_from));
    extract_header_value(buffer, buffer_len, HEADER_KOSPAM_ENVELOPE_RECIPIENT, strlen(HEADER_KOSPAM_ENVELOPE_RECIPIENT), m->kospam.kospam_envelope_recipient, sizeof(m->kospam.kospam_envelope_recipient));
    extract_header_value(buffer, buffer_len, HEADER_KOSPAM_XFORWARD, strlen(HEADER_KOSPAM_XFORWARD), m->kospam.kospam_xforward, sizeof(m->kospam.kospam_xforward));
    extract_header_value(buffer, buffer_len, HEADER_KOSPAM_WATERMARK, strlen(HEADER_KOSPAM_WATERMARK), m->header.kospam_watermark, sizeof(m->header.kospam_watermark));

    // The Received: lines require special care
    // Sometimes kospam runs not on the MX server, but only after
    // In this case we want to skip the first Received: lines to
    // get the real smtp client that sent us the email

    char *p = buffer;
    int i = 0;
    while ((p = strcasestr(p, HEADER_RECEIVED))) {
       extract_header_value(p, strlen(p), HEADER_RECEIVED, strlen(HEADER_RECEIVED), m->header.received, sizeof(m->header.received));
       if (i == cfg->received_lines_to_skip) break;
       i++;
       p += strlen(HEADER_RECEIVED);
    }

    // Check for our backscatter signo

    if (cfg->our_signo[0] && \
        (strstr(m->kospam.kospam_envelope_from, "MAILER-DAEMON") || strstr(m->kospam.kospam_envelope_from, "<>"))) {
        m->need_signo_check = true;
        if (strstr(buffer, cfg->our_signo)) {
           m->found_our_signo = true;
        }
    }

    if(!headers_end) return 1;

    *headers_end = '\n';

    char *body = headers_end + body_offset;
    // multipart?
    if (m->header.content_type[0] && strstr(m->header.content_type, "multipart/")) {
       char boundary[SMALLBUFSIZE];
       if(!find_boundary(m->header.content_type, boundary, sizeof(boundary))) {
          return 1; // TODO: error handling
       }

       extract_mime_parts(body, boundary, m, cfg);
    }
    else {
       // Not multipart, no boundary

       // Check if we need to decode
       bool needs_base64_decode = false;
       bool needs_quoted_printable_decode = false;

       if (m->header.content_encoding[0]) {
          if(strcasestr(m->header.content_encoding, "base64")) {
             needs_base64_decode = true;
          } else if(strcasestr(m->header.content_encoding, "quoted-printable")) {
             needs_quoted_printable_decode = true;
          }
       }

       // by default it's textual
       bool textual_part = true;

       if (m->header.content_type[0] && !strstr(m->header.content_type, "text/plain") && !strstr(m->header.content_type, "text/html") ) {
           textual_part = false;
       }

       if (textual_part) {
          if (needs_base64_decode) {
              base64_decode(body);
          } else if (textual_part && needs_quoted_printable_decode) {
              decodeQP(body);
          }

          if (strcasestr(m->header.content_type, "text/html")) {
             normalize_html(body);
          }

          APPENDTOBODY(body, m->body.data, m->body.pos);
       }

    }

    return 0;
}


void extract_mime_parts(char *body, const char *boundary, struct Message *m, struct config *cfg) {
   char boundary_marker[SMALLBUFSIZE];
   snprintf(boundary_marker, sizeof(boundary_marker)-1, "--%s", boundary);

   //printf("boundary marker: %s\n", boundary_marker);

   char *part = strstr(body, boundary_marker);
   while (part) {
      part = part + strlen(boundary_marker);

      //printf("NEW PART\n");

      // Find the headers of this part
      const char *part_headers = part;

      // Skip to the next boundary or end
      char *next_part = strstr(part, boundary_marker);
      if (next_part) *next_part = '\0';

      const char *part_content_type = strcasestr(part_headers, "Content-Type:");
      if (part_content_type) {

         char charset[SMALLBUFSIZE];
         memset(charset, 0, sizeof(charset));

         // Find the body of this part (after the headers)
         int part_body_offset = 4;
         char *part_body = strstr(part_headers, "\r\n\r\n");
         if (!part_body) {
            part_body = strstr(part_headers, "\n\n");
            part_body_offset = 2;
         }
         if (!part_body) continue;

         part_body += part_body_offset;

         // what if we have another multipart in the MIME part?

         char part_content_type_buf[SMALLBUFSIZE];
         memset(part_content_type_buf, 0, sizeof(part_content_type_buf));

         char content_type[2*SMALLBUFSIZE];
         extract_header_value(part_headers, strlen(part_headers), HEADER_CONTENT_TYPE, strlen(HEADER_CONTENT_TYPE), &content_type[0], sizeof(content_type));

         if (content_type[0]) {
            snprintf(part_content_type_buf, sizeof(part_content_type_buf)-1, "%s", content_type);
            extract_token_from_header_line(content_type, "charset", charset, sizeof(charset));
         }

         if (content_type[0] && strcasestr(content_type, "multipart/")) {
            char boundary[SMALLBUFSIZE];
            if (!find_boundary(content_type, boundary, sizeof(boundary))) return; // TODO: error handling

            extract_mime_parts(part_body, boundary, m, cfg);
         }
         else {
            char c = *part_body;
            *part_body = '\0';

            //printf("PART HDR=%s", part);

            /* For non textual parts the attachment name might be important, eg.
             *
             * Content-Type: image/jpeg;
             *      name="ubaxxezoeycayr.jpeg"
             *
             * or
             *
             * Content-Type: application/octet-stream;
             *         name="Magnetic Filter.PDF.pdf"
             * Content-Transfer-Encoding: base64
             * Content-Disposition: attachment;
             *        filename="Magnetic Filter.PDF.pdf"
             */

            // Check if we need to decode
            bool needs_base64_decode = false;
            bool needs_quoted_printable_decode = false;

            char part_headers_buf[SMALLBUFSIZE];
            snprintf(part_headers_buf, sizeof(part_headers_buf)-1, "%s", part_headers);

            const char *transfer_encoding = strcasestr(part_headers_buf, HEADER_CONTENT_TRANSFER_ENCODING);
            if (transfer_encoding) {
               if(strcasestr(transfer_encoding, "base64")) {
                  needs_base64_decode = true;
               } else if(strcasestr(transfer_encoding, "quoted-printable")) {
                  needs_quoted_printable_decode = true;
               }
            }

            bool textual_part = false;
            bool html = false;
            bool rfc822 = false;

            if (strcasestr(part_content_type, "text/plain") ||
                strcasestr(part_content_type, "text/calendar") ||
                strcasestr(part_content_type, "application/ics")
            ) {
                textual_part = true;
            } else if (strcasestr(part_content_type, "text/html")) {
                textual_part = true;
                html = true;
            } else if (strcasestr(part_content_type, "message/rfc822")) {
                rfc822 = true;
            } else if(strcasestr(part_content_type, "application/octet-stream")) {
                m->has_octet_stream_attachment = 1;
            } else if(strcasestr(part_content_type, "image/")) {
                m->has_image_attachment = 1;
            }


            *part_body = c;

            if (textual_part) {
               char filename[SMALLBUFSIZE];
               extract_name_from_headers(part_headers_buf, filename, sizeof(filename));

               bool want_to_process_this_mime_part = true;

               // I dont want textual attachments, eg. .txt file if it's larger than 30 kB
               if (filename[0]) {
                   //printf("DEBUG: textual attachment: **%s**\n", filename);
                   size_t part_body_len = strlen(part_body);
                   if (part_body_len > 30000) {
                       want_to_process_this_mime_part = false;
                   }
               }

               if (want_to_process_this_mime_part) {
                   if (needs_base64_decode) {
                       base64_decode(part_body);
                   } else if (textual_part && needs_quoted_printable_decode) {
                       decodeQP(part_body);
                   }

                   if (!strcmp(charset, "windows-1251")) {
                       decode_html_entities_utf8_inplace(part_body);
                   }

                   if (html) {
                       normalize_html(part_body);
                   }

                   APPENDTOBODY(part_body, m->body.data, m->body.pos);
               }

            } else if (rfc822) {
               // drop previous email headers
               memset((char*)&(m->header), 0, sizeof(struct Header));
               parse_eml_buffer(part_body, m, cfg);
            }
            else {
               // Get the filename
               char filename[SMALLBUFSIZE];
               extract_name_from_headers(part_headers_buf, filename, sizeof(filename));

               if (m->n_attachments < MAX_ATTACHMENTS) {
                   CONVERT_WHITESPACE_TO_UNDERSCORE(filename);
                   snprintf(m->attachments[m->n_attachments].filename, sizeof(m->attachments[m->n_attachments].filename)-1, "ATT*%s", filename);
                   APPENDTOBODY(m->attachments[m->n_attachments].filename, m->body.data, m->body.pos);

                   m->attachments[m->n_attachments].size = strlen(part_body);

                   chop_newlines(part_body, m->attachments[m->n_attachments].size);
                   digest_string("sha256", part_body, &(m->attachments[m->n_attachments].digest[0]) );

                   char *p = strchr(part_content_type_buf, ';');
                   if (p) {
                      *p = '\0';
                      snprintf(m->attachments[m->n_attachments].type, sizeof(m->attachments[m->n_attachments].type)-1, "%s", part_content_type_buf);
                   }

                   //printf("MIME BODY nontext: ***%s***", part_body);

                   m->n_attachments++;
               }
            }
         }
      }

      part = next_part;
   }

}


int find_boundary(const char *buffer, char *result, size_t resultlen) {
    memset(result, 0, resultlen);

    const char *boundary = strstr(buffer, "boundary");
    if (!boundary) return 0;

    boundary += strlen("boundary");

    // I've seen an idiot spammer using the following boundary definition in the header:
    //
    // Content-Type: multipart/alternative; boundary=3D"b1_52b92b01a943615aff28b7f4d2f2d69d"
    //
    if (!strncmp(boundary, "=3D", 3)) {
        boundary += 3;
    }

    // what if boundary = "..." instead of boundary="..."

    while (isspace(*boundary)) boundary++;
    if (*boundary == '=') boundary++;
    while (isspace(*boundary)) boundary++;

    const char *end;

    if (*boundary == '"') {
        // Quoted boundary
        boundary++;
        end = strchr(boundary, '"');
        if (!end) return 0;
    }
    else {
        // Unquoted boundary
        end = boundary;
        while (*end && !isspace(*end) && *end != ';') end++;
    }

    size_t len = end - boundary;

    if (len > resultlen - 1) len = resultlen - 1;
    memcpy(result, boundary, len);

    return 1;
}
