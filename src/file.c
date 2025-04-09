#include <kospam.h>

char* read_file(const char* filename, size_t* size_out) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        syslog(LOG_PRIORITY, "ERROR: failed to open %s", filename);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read entire file into buffer
    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        syslog(LOG_PRIORITY, "ERROR: memory allocation failed");
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);

    if (bytes_read != file_size) {
        free(buffer);
        syslog(LOG_PRIORITY, "ERROR: read error: expected %zu bytes, got %zu bytes", file_size, bytes_read);
        return NULL;
    }

    buffer[file_size] = '\0';  // Null-terminate if you need it as a string
    if (size_out) *size_out = file_size;

    return buffer;
}


int write_buffers_to_file(const char* filename, const char* buffer1, size_t size1, const char* buffer2, size_t size2) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        syslog(LOG_PRIORITY, "ERROR: failed to open %s for writing", filename);
        return -1;
    }

    // Write first buffer
    size_t written1 = fwrite(buffer1, 1, size1, file);
    if (written1 != size1) {
        syslog(LOG_PRIORITY, "ERROR: failed to write first buffer to %s", filename);
        fclose(file);
        return -1;
    }

    if (buffer2) {
        // Write second buffer, the body
        size_t written2 = fwrite(buffer2, 1, size2, file);
        if (written2 != size2) {
            syslog(LOG_PRIORITY, "ERROR: failed to write second buffer to %s", filename);
            fclose(file);
            return -1;
        }
    }

    // Close file and check for errors
    if (fclose(file) != 0) {
        syslog(LOG_PRIORITY, "ERROR: closing file %s", filename);
        return -1;
    }

    return 0; // Success
}


int fix_message_file(const char *filename, struct session_data *sdata, struct __config *cfg) {
    size_t size_out = 0;
    char *buffer = read_file(filename, &size_out);

    if (!buffer) return 1;

    size_t body_size = 0;

    char *headers_end = strstr(buffer, "\r\n\r\n");
    if (!headers_end) {
       headers_end = strstr(buffer, "\n\n");
    }

    if (headers_end) {
       body_size = size_out - (headers_end - buffer);
       *headers_end = '\0';
    }

    char *q = buffer;

    int my_header = 0;
    char headerbuf[MAX_MAIL_HEADER_SIZE+MAXBUFSIZE];
    memset(headerbuf, 0, sizeof(headerbuf));
    int headerbuf_pos = 0;

    int spaminess_header_len = strlen(cfg->clapf_header_field);

    int i = 0;

    while (q) {
       i++;
       int result;
       char v[SMALLBUFSIZE];
       q = split(q, '\n', v, sizeof(v), &result);

       int len = strlen(v);
       int add_to_header = 0;

       // folded header line
       if( v[0] == ' ' || v[0] == '\t' ) {
          if (my_header == 1) {
             add_to_header = 0;
          } else {
             add_to_header = 1;
          }
       }
       else {
          if(strncmp(v, cfg->clapf_header_field, spaminess_header_len) == 0) {
             my_header = 1;
             add_to_header = 0;
          }
          else {
             my_header = 0;
             add_to_header = 1;
          }
       }

       // Write only HEADER_BUF_SIZE bytes to header, and discard the rest

       if (len + headerbuf_pos < MAX_MAIL_HEADER_SIZE && add_to_header) {
          memcpy(headerbuf+headerbuf_pos, v, len);
          headerbuf_pos += len;
          memcpy(headerbuf+headerbuf_pos, "\n", 1);
          headerbuf_pos++;
       }

    }
    // Add our spaminessbuf. No trailing \r\n!
    //char *spaminessbuf = "X-Kospam-Result: 4000000123456789\r\nX-Kospam-Result: 0.9992\r\nX-Kospam-Result: Yes";
    int len = strlen(sdata->spaminessbuf);

    memcpy(headerbuf+headerbuf_pos, sdata->spaminessbuf, len);
    headerbuf_pos += len;

    // Beware of header only messages
    if (headers_end) *headers_end = '\n';

    // Write to temp file
    char tmp[SMALLBUFSIZE];
    snprintf(tmp, sizeof(tmp)-1, "%s.tmp", filename);
    write_buffers_to_file(tmp, headerbuf, headerbuf_pos, headers_end, body_size);

    free(buffer);

    // Rename temp file to filename
    if(rename(tmp, filename)) {
       syslog(LOG_PRIORITY, "ERROR: failed to rename %s to %s", tmp, filename);
    }

    return 0;
}
