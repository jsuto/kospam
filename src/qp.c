#include <kospam.h>

static char hex_table[] = {
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  1,  2,  3,   4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,

   0, 10, 11, 12,  13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0, 10, 11, 12,  13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};


void decodeQP(char *p) {
    unsigned int i;
    int k = 0;
    unsigned char a, b;
    char c;

    if(p == NULL) return;

    size_t len = strlen(p);

    for(i = 0; i < len; i++) {
        c = p[i];

        if(c == '=' && i+2 < strlen(p) && isxdigit(p[i+1]) && isxdigit(p[i+2])) {
            // Handle hex-encoded character
            a = p[i+1];
            b = p[i+2];
            c = 16 * hex_table[a] + hex_table[b];
            i += 2;
        }
        else if(c == '=' && i+1 < strlen(p) && p[i+1] == '\r' && i+2 < strlen(p) && p[i+2] == '\n') {
            // Handle soft line break (=\r\n), skip it
            i += 2;
            continue;
        }
        else if(c == '=' && i+1 < strlen(p) && p[i+1] == '\n') {
            // Handle soft line break (=\n), skip it
            i += 1;
            continue;
        }
        else if(c == '_') {
            // RFC 2047 special case for encoded-word: underscore represents space
            c = ' ';
        }

        p[k++] = c;
    }

    p[k] = '\0';
}
