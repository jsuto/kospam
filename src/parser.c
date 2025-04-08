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


void init_state(struct __state *state){
   memset((char*)state, 0, sizeof(*state)); // sizeof(state) is only 8 bytes!

   state->tre = '-';

   inithash(state->token_hash);
   inithash(state->url);
}

int parse_message(const char *message, struct __state *state, struct Message *m) {
    size_t s;
    char *buffer = read_file(message, &s);

    if (!buffer) {
        syslog(LOG_PRIORITY, "ERROR: failed to read %s", message);
        return 1;
    }

    memset((char *)m, 0, sizeof(*m));

    parse_eml_buffer(buffer, m);

    free(buffer);

    init_state(state);

    utf8_tolower(m->body.data);
    normalize_buffer(m->body.data);

    state->has_image_attachment = m->has_image_attachment;
    state->has_octet_stream_attachment = m->has_octet_stream_attachment;

    return 0;
}

int post_parse(struct __state *state, struct Message *m, struct __config *cfg) {

    state->n_token = generate_tokens_from_string(state, m->body.data, "", cfg);

    if (m->header.subject[0]) {
        state->n_subject_token = generate_tokens_from_string(state, m->header.subject, "SUBJ*", cfg);
        state->n_token += generate_tokens_from_string(state, m->header.subject, "", cfg);
        snprintf(state->b_subject, sizeof(state->b_subject)-1, "%s", m->header.subject);
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

    if (m->header.kospam_envelope_from[0]) {
        snprintf(state->fromemail, sizeof(state->fromemail)-1, "%s", m->header.kospam_envelope_from);
    }

    if (m->header.kospam_envelope_recipient[0]) {
        char *p = m->header.kospam_envelope_recipient;

        while (p) {
            int result;
            char v[SMALLBUFSIZE];
            p = split(p, ',', v, sizeof(v)-1, &result);
            if (strstr(v, "spam@") ||
                strstr(v, "+spam@") ||
                strstr(v, "ham@") ||
                strstr(v, "+ham@")
            ) {
                state->training_request = 1;
            }
        }
    }

    if (m->header.kospam_xforward[0]) {
        // Data is expected in this order
        // NAME=smtp.example.com ADDR=1.2.3.4 PROTO=ESMTP HELO=smtp.example.com

        int i = 0;
        char *p = m->header.kospam_xforward;

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

    if (!m->header.kospam_xforward[0] && m->header.received[0]) {
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
                    if(is_item_on_list(q, cfg->skipped_received_ips, "127.,10.,192.168.,172.16.") == 0) {
                       snprintf(state->ip, sizeof(state->ip)-1, "%s", q);
                    }
                }
                else {
                    snprintf(state->hostname, sizeof(state->hostname)-1, "%s", q);
                }
            } else if(strcmp(v, "unknown")) {
                addnode(state->token_hash, "UNKNOWN_CLIENT*", REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
            }
        }
    }

    if (cfg->debug == 1) {
        printf("From: %s\n", m->header.from);
        printf("Message-ID: %s\n", m->header.message_id);
        printf("Subject: %s\n", m->header.subject);
        printf("Content-type: %s\n", m->header.content_type);
        printf("Content-Transfer-Encoding: %s\n", m->header.content_encoding);
        printf("Received: %s\n", m->header.received);
        printf("Kospam-Envelope-From: %s\n", m->header.kospam_envelope_from);
        printf("Kospam-Envelope-Recipient: %s\n", m->header.kospam_envelope_recipient);
        printf("Kospam-Xforward: %s\n", m->header.kospam_xforward);

        for(int i=0; i < m->n_attachments; i++){
            printf("i=%d, name=%s, type=%s, size=%ld, digest=%s\n", i, m->attachments[i].filename, m->attachments[i].type, m->attachments[i].size, m->attachments[i].digest);
        }

        printf("\n\nBODY: %s\n", m->body.data);
    }


    return 0;
}


int parse_eml_buffer(char *buffer, struct Message *m) {
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
    extract_header_value(buffer, buffer_len, HEADER_RECEIVED, strlen(HEADER_RECEIVED), m->header.received, sizeof(m->header.received));

    // Kospam-* headers
    extract_header_value(buffer, buffer_len, HEADER_KOSPAM_ENVELOPE_FROM, strlen(HEADER_KOSPAM_ENVELOPE_FROM), m->header.kospam_envelope_from, sizeof(m->header.kospam_envelope_from));
    extract_header_value(buffer, buffer_len, HEADER_KOSPAM_ENVELOPE_RECIPIENT, strlen(HEADER_KOSPAM_ENVELOPE_RECIPIENT), m->header.kospam_envelope_recipient, sizeof(m->header.kospam_envelope_recipient));
    extract_header_value(buffer, buffer_len, HEADER_KOSPAM_XFORWARD, strlen(HEADER_KOSPAM_XFORWARD), m->header.kospam_xforward, sizeof(m->header.kospam_xforward));

    if(!headers_end) return 1;

    *headers_end = '\n';

    char *body = headers_end + body_offset;
    // multipart?
    if (m->header.content_type[0] && strstr(m->header.content_type, "multipart/")) {
       char *boundary = find_boundary(m->header.content_type);

       if (!boundary) return 1; // TODO: error handling

       extract_mime_parts(body, boundary, m);

       free(boundary);

    } else {
       // Not multipart, no boundary

       //printf("INFO: not multipart\n");

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


void extract_mime_parts(char *body, const char *boundary, struct Message *m) {
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
            extract_name_from_header_line(content_type, "charset", charset, sizeof(charset));
         }

         if (content_type[0] && strcasestr(content_type, "multipart/")) {
            char *boundary = find_boundary(content_type);

            if (!boundary) return; // TODO: error handling

            extract_mime_parts(part_body, boundary, m);

            free(boundary);
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

            const char *transfer_encoding = strcasestr(part_headers, HEADER_CONTENT_TRANSFER_ENCODING);
            if (transfer_encoding) {
               if(strstr(transfer_encoding, "base64")) {
                  needs_base64_decode = true;
               } else if(strstr(transfer_encoding, "quoted-printable")) {
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


            char part_headers_buf[SMALLBUFSIZE];
            snprintf(part_headers_buf, sizeof(part_headers_buf)-1, "%s", part_headers);

            *part_body = c;

            if (textual_part) {
               if (needs_base64_decode) {
                  base64_decode(part_body);
               } else if (textual_part && needs_quoted_printable_decode) {
                  decodeQP(part_body);
               }

               if(!strcmp(charset, "windows-1251")) {
                  decode_html_entities_utf8_inplace(part_body);
               }

               if (html) {
                  normalize_html(part_body);
               }

               APPENDTOBODY(part_body, m->body.data, m->body.pos);

            } else if (rfc822) {
               // drop previous email headers
               memset((char*)&(m->header), 0, sizeof(struct Header));
               parse_eml_buffer(part_body, m);
            }
            else {
               // Get the filename
               char filename[SMALLBUFSIZE];
               //printf("aa=**%s**\n", part_headers_buf);

               char *content_disposition = strcasestr(part_headers_buf, HEADER_CONTENT_DISPOSITION);
               if (content_disposition) {
                  extract_name_from_header_line(content_disposition, "name", filename, sizeof(filename));
               } else {
                  extract_name_from_header_line(part_headers_buf, "name", filename, sizeof(filename));
               }

               if (m->n_attachments < MAX_ATTACHMENTS) {
                   CONVERT_WHITESPACE_TO_UNDERSCORE(filename);
                   snprintf(m->attachments[m->n_attachments].filename, sizeof(m->attachments[m->n_attachments].filename)-1, "ATT*%s ", filename);
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


char *find_boundary(const char *buffer) {

    const char *boundary = strstr(buffer, "boundary");
    if (!boundary) return NULL;

    boundary += strlen("boundary");

    // what if boundary = "..." instead of boundary="..."

    while (isspace(*boundary)) boundary++;
    if (*boundary == '=') boundary++;
    while (isspace(*boundary)) boundary++;

    // Check if boundary is quoted
    if (*boundary == '"') {
        boundary++;
        const char *end = strchr(boundary, '"');
        if (!end) return NULL;

        size_t len = end - boundary;
        char *result = (char *)malloc(len + 1);
        if (!result) return NULL;

        memcpy(result, boundary, len);
        result[len] = '\0';
        return result;
    } else {
        // Unquoted boundary
        const char *end = boundary;
        while (*end && !isspace(*end) && *end != ';') end++;

        size_t len = end - boundary;
        char *result = (char *)malloc(len + 1);
        if (!result) return NULL;

        memcpy(result, boundary, len);
        result[len] = '\0';
        return result;
    }
}
