#include <kospam.h>

// Encode a single Unicode codepoint into UTF-8 at *dst
// Returns number of bytes written
int encode_utf8(char *dst, unsigned int codepoint) {
    if (codepoint <= 0x7F) {
        dst[0] = codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        dst[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
        dst[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    } else if (codepoint <= 0xFFFF) {
        dst[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
        dst[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    } else if (codepoint <= 0x10FFFF) {
        dst[0] = 0xF0 | ((codepoint >> 18) & 0x07);
        dst[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        dst[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[3] = 0x80 | (codepoint & 0x3F);
        return 4;
    }
    // Invalid codepoint, ignore
    return 0;
}

void decode_html_entities_utf8_inplace(char *buffer) {
    char *src = buffer;
    char *dst = buffer;

    while (*src) {
        if (src[0] == '&' && src[1] == '#') {
            int is_hex = (src[2] == 'x' || src[2] == 'X');
            char *start = src + (is_hex ? 3 : 2);
            char *end;
            long codepoint = strtol(start, &end, is_hex ? 16 : 10);

            if (*end == ';') {
                int len = encode_utf8(dst, (unsigned int)codepoint);
                dst += len;
                src = end + 1;
                continue;
            }
        }

        *dst++ = *src++;
    }

    *dst = '\0'; // Null-terminate
}
