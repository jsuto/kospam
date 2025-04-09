#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stddef.h>

// Base64 decoding lookup table
static const unsigned char base64_dec_table[256] = {
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 62, 80, 80, 80, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 80, 80, 80, 0, 80, 80,
    80,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 80, 80, 80, 80, 80,
    80, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80
};

#define VALID_BASE64_CHAR(c) (c >= 'A' && c <= 'Z') || \
                             (c >= 'a' && c <= 'z') || \
                             (c >= '0' && c <= '9') || \
                             c == '+' || c == '/' || c == '=' ? 1 : 0

/**
 * Decodes a base64 string in-place
 *
 * @param input The base64 encoded string (null-terminated)
 * @return The length of the decoded data, or -1 on error
 */

int base64_decode(char *input) {
    if (input == NULL) return -1;

    size_t input_len = strlen(input);
    if (input_len == 0) return 0;

    // First pass: Keep only valid base64 characters
    size_t clean_len = 0;
    for (size_t i = 0; i < input_len; i++) {
        if (VALID_BASE64_CHAR(input[i])) {
            input[clean_len++] = input[i];
        }
    }
    input[clean_len] = '\0';

    // Check for valid base64 length after cleanup (must be divisible by 4)
    if (clean_len % 4 != 0 && clean_len > 0) return -1;

    // Count padding characters
    int padding = 0;
    if (clean_len > 0) {
        if (input[clean_len - 1] == '=') padding++;
        if (clean_len > 1 && input[clean_len - 2] == '=') padding++;
    }

    // Calculate output length
    size_t output_len = (clean_len / 4) * 3 - padding;

    size_t i, j;
    unsigned char a, b, c, d;

    // Process input in groups of 4 characters
    for (i = 0, j = 0; i < clean_len - padding; i += 4, j += 3) {
        a = base64_dec_table[(unsigned char)input[i]];
        b = base64_dec_table[(unsigned char)input[i + 1]];
        c = (i + 2 < clean_len) ? base64_dec_table[(unsigned char)input[i + 2]] : 0;
        d = (i + 3 < clean_len) ? base64_dec_table[(unsigned char)input[i + 3]] : 0;

        // Check for invalid characters
        if (a > 63 || b > 63 || c > 63 || d > 63) { printf("XXX %d, %d, %d, %d => %c%c%c%c\n", a, b, c, d, input[i], input[i+1], input[i+2], input[i+3]); return -1; }

        // Decode group of 4 characters into 3 bytes
        input[j] = (a << 2) | (b >> 4);

        if (i + 2 < clean_len - padding) {
            input[j + 1] = (b << 4) | (c >> 2);
        }

        if (i + 3 < clean_len - padding) {
            input[j + 2] = (c << 6) | d;
        }
    }

    // Null-terminate the decoded data
    input[output_len] = '\0';

    return (int)output_len;
}
