#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>


/*
 * split a string by a character as delimiter
 */

char *split(char *str, int ch, char *buf, int buflen, int *result) {
    char *p;

    memset(buf, 0, buflen);

    /* Initialize result and validate parameters */
    *result = 0;
    if (str == NULL || buf == NULL || buflen < 1) {
        return NULL;
    }

    /* If input string is empty, return NULL */
    if (*str == '\0') {
        buf[0] = '\0';
        return NULL;
    }

    /* Find the delimiter */
    p = strchr(str, ch);

    if (p) {
        /* Calculate token length */
        size_t token_len = p - str;

        /* Copy at most buflen-1 characters to ensure space for null terminator */
        size_t copy_len = (token_len < (size_t)buflen - 1) ? token_len : (size_t)buflen - 1;
        memcpy(buf, str, copy_len);
        buf[copy_len] = '\0';

        /* Set success flag and return pointer to next position */
        *result = 1;
        return p + 1;
    } else {
        /* No delimiter found, copy the entire remaining string */
        strncpy(buf, str, buflen - 1);
        buf[buflen - 1] = '\0';

        /* Set success flag if we copied something */
        *result = (*str != '\0');
        return NULL;
    }
}
