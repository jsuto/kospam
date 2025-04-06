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


Email *email;
int has_image_attachment;
int has_octet_stream_attachment;
int n_attachments;
struct attachment attachments[MAX_ATTACHMENTS];

void init_state(struct __state *state){
   memset((char*)state, 0, sizeof(*state)); // sizeof(state) is only 8 bytes!

   state->tre = '-';

   inithash(state->token_hash);
   inithash(state->url);
}

int parse_message(const char *message, struct __state *state, struct Body *BODY) {
    size_t s;
    char *buffer = read_file(message, &s);

    if (!buffer) {
        syslog(LOG_PRIORITY, "ERROR: failed to read %s", message);
        return 1;
    }

    memset((char *)BODY, 0, sizeof(BODY));
    memset((char *)attachments, 0, sizeof(attachments));
    n_attachments = 0;
    has_image_attachment = 0;
    has_octet_stream_attachment = 0;

    parse_eml_buffer(buffer, BODY);

    free(buffer);

    if (!email) {
        syslog(LOG_PRIORITY, "ERROR: failed to parse buffer");
        return 1;
    }

    init_state(state);

    utf8_tolower(BODY->data);
    normalize_buffer(BODY->data);

    return 0;
}

int post_parse(struct __state *state, struct Body *BODY, struct __config *cfg) {

    state->n_token = generate_tokens_from_string(state, BODY->data, "", cfg);

    if (email->subject) {
        state->n_subject_token = generate_tokens_from_string(state, email->subject, "SUBJ*", cfg);
        state->n_token += generate_tokens_from_string(state, email->subject, "", cfg);
    }

    if (email->from) {
        generate_tokens_from_string(state, email->from, "HEADER*", cfg);

        char tmp[SMALLBUFSIZE];
        snprintf(tmp, sizeof(tmp)-1, "FROM*%s", email->from);
        addnode(state->token_hash, tmp, DEFAULT_SPAMICITY, 0);

        // Add the From: domain as a token
        char *p = strchr(email->from, '@');
        if (p) generate_tokens_from_string(state, p, "HEADER*", cfg);
    }

    if (email->subject) {
        snprintf(state->b_subject, sizeof(state->b_subject)-1, "%s", email->subject);
    }

    if (!email->message_id) {
        addnode(state->token_hash, "NO_MESSAGE_ID*",  REAL_SPAM_TOKEN_PROBABILITY, DEVIATION(REAL_SPAM_TOKEN_PROBABILITY));
    }

    if (email->kospam_envelope_from) {
        snprintf(state->fromemail, sizeof(state->fromemail)-1, "%s", email->kospam_envelope_from);
    }

    if (email->kospam_envelope_recipient) {
        char *p = email->kospam_envelope_recipient;

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

    if (email->kospam_xforward) {
        // Data is expected in this order
        // NAME=smtp.example.com ADDR=1.2.3.4 PROTO=ESMTP HELO=smtp.example.com

        int i = 0;
        char *p = email->kospam_xforward;

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

    if (!(email->kospam_xforward) && email->received) {
        char *p = strstr(email->received, " by ");
        if (p) *p = '\0';

        // from mail.trops.eu (mail.trops.eu [37.220.140.203])
        p = email->received;
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

    if (email) {
        if (cfg->debug == 1) {
            printf("From: %s\n", email->from ? email->from : "N/A");
            printf("Message-ID: %s\n", email->message_id ? email->message_id : "N/A");
            printf("Subject: %s\n", email->subject ? email->subject : "N/A");
            printf("Content-type: %s\n", email->content_type ? email->content_type : "N/A");
            printf("Content-Transfer-encoding: %s\n", email->content_encoding ? email->content_encoding : "N/A");
            printf("Received: %s\n", email->received ? email->received : "N/A");
            printf("Kospam-Envelope-From: %s\n", email->kospam_envelope_from ? email->kospam_envelope_from : "N/A");
            printf("Kospam-Envelope-Recipient: %s\n", email->kospam_envelope_recipient ? email->kospam_envelope_recipient : "N/A");
            printf("Kospam-Xforward: %s\n", email->kospam_xforward ? email->kospam_xforward : "N/A");

            for(int i=0; i<n_attachments; i++){
                printf("i=%d, name=%s, type=%s, size=%ld, digest=%s\n", i, attachments[i].filename, attachments[i].type, attachments[i].size, attachments[i].digest);
            }

            printf("\n\nBODY: %s\n", BODY->data);
        }

        free_email(email);
    }

    return 0;
}


void free_email(Email *email) {
    if (!email) return;

    if (email->from) free(email->from);
    if (email->subject) free(email->subject);
    if (email->message_id) free(email->message_id);
    if (email->content_type) free(email->content_type);
    if (email->content_encoding) free(email->content_encoding);
    if (email->received) free(email->received);

    free(email);
}

int parse_eml_buffer(char *buffer, struct Body *b) {
    // Create and populate Email structure
    email = (Email *)calloc(1, sizeof(Email));
    if (!email) {
        return 1;
    }

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

    email->from = extract_header_value(buffer, buffer_len, HEADER_FROM, strlen(HEADER_FROM));
    email->subject = extract_header_value(buffer, buffer_len, HEADER_SUBJECT, strlen(HEADER_SUBJECT));
    email->message_id = extract_header_value(buffer, buffer_len, HEADER_MESSAGE_ID, strlen(HEADER_MESSAGE_ID));
    email->content_type = extract_header_value(buffer, buffer_len, HEADER_CONTENT_TYPE, strlen(HEADER_CONTENT_TYPE));
    email->content_encoding = extract_header_value(buffer, buffer_len, HEADER_CONTENT_TRANSFER_ENCODING, strlen(HEADER_CONTENT_TRANSFER_ENCODING));
    email->received = extract_header_value(buffer, buffer_len, HEADER_RECEIVED, strlen(HEADER_RECEIVED));

    // Kospam-* headers
    email->kospam_envelope_from = extract_header_value(buffer, buffer_len, HEADER_KOSPAM_ENVELOPE_FROM, strlen(HEADER_KOSPAM_ENVELOPE_FROM));
    email->kospam_envelope_recipient = extract_header_value(buffer, buffer_len, HEADER_KOSPAM_ENVELOPE_RECIPIENT, strlen(HEADER_KOSPAM_ENVELOPE_RECIPIENT));
    email->kospam_xforward = extract_header_value(buffer, buffer_len, HEADER_KOSPAM_XFORWARD, strlen(HEADER_KOSPAM_XFORWARD));

    if(!headers_end) return 1;

    *headers_end = '\n';

    char *body = headers_end + body_offset;
    // multipart?
    if (email->content_type && strstr(email->content_type, "multipart/")) {
       char *boundary = find_boundary(email->content_type);

       if (!boundary) return 1; // TODO: error handling

       extract_mime_parts(body, boundary, b);

       free(boundary);

    } else {
       // Not multipart, no boundary

       //printf("INFO: not multipart\n");

       // Check if we need to decode
       bool needs_base64_decode = false;
       bool needs_quoted_printable_decode = false;

       if (email->content_encoding) {
          //printf("INFO: encoding=%s\n", email->content_encoding);

          if(strcasestr(email->content_encoding, "base64")) {
             needs_base64_decode = true;
          } else if(strcasestr(email->content_encoding, "quoted-printable")) {
             needs_quoted_printable_decode = true;
          }
       }

       // by default it's textual
       bool textual_part = true;

       if (email->content_type && !strstr(email->content_type, "text/plain") && !strstr(email->content_type, "text/html") ) {
           textual_part = false;
       }

       if (textual_part) {
          if (needs_base64_decode) {
              base64_decode(body);
          } else if (textual_part && needs_quoted_printable_decode) {
              decodeQP(body);
          }

          if (strcasestr(email->content_type, "text/html")) {
             normalize_html(body);
          }

          APPENDTOBODY(body, b->data, b->pos);
       }

    }

    return 0;
}


void extract_mime_parts(char *body, const char *boundary, struct Body *b) {
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

         char *content_type = extract_header_value(part_headers, strlen(part_headers), HEADER_CONTENT_TYPE, strlen(HEADER_CONTENT_TYPE));

         if (content_type) {
            snprintf(part_content_type_buf, sizeof(part_content_type_buf)-1, "%s", content_type);
            extract_name_from_header_line(content_type, "charset", charset, sizeof(charset));
         }

         if (content_type && strcasestr(content_type, "multipart/")) {
            char *boundary = find_boundary(content_type);

            free(content_type);

            if (!boundary) return; // TODO: error handling

            extract_mime_parts(part_body, boundary, b);

            free(boundary);
         }
         else {
            if (content_type) free(content_type);

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
                has_octet_stream_attachment = 1;
            } else if(strcasestr(part_content_type, "image/")) {
                has_image_attachment = 1;
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

               APPENDTOBODY(part_body, b->data, b->pos);

            } else if (rfc822) {
               // drop previous email headers

               if (email) {
                  free_email(email);
               }

               parse_eml_buffer(part_body, b);
            } else {
               // Get the filename
               char filename[SMALLBUFSIZE];
               //printf("aa=**%s**\n", part_headers_buf);

               char *content_disposition = strcasestr(part_headers_buf, HEADER_CONTENT_DISPOSITION);
               if (content_disposition) {
                  extract_name_from_header_line(content_disposition, "name", filename, sizeof(filename));
               } else {
                  extract_name_from_header_line(part_headers_buf, "name", filename, sizeof(filename));
               }

               if (n_attachments < MAX_ATTACHMENTS) {
                   CONVERT_WHITESPACE_TO_UNDERSCORE(filename);
                   snprintf(attachments[n_attachments].filename, sizeof(attachments[n_attachments].filename)-1, "ATT*%s ", filename);
                   APPENDTOBODY(attachments[n_attachments].filename, b->data, b->pos);

                   attachments[n_attachments].size = strlen(part_body);

                   chop_newlines(part_body, attachments[n_attachments].size);
                   digest_string("sha256", part_body, &(attachments[n_attachments].digest[0]) );

                   char *p = strchr(part_content_type_buf, ';');
                   if (p) {
                      *p = '\0';
                      snprintf(attachments[n_attachments].type, sizeof(attachments[n_attachments].type)-1, "%s", part_content_type_buf);
                   }

                   //printf("MIME BODY nontext: ***%s***", part_body);

                   n_attachments++;
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
