#include <kospam.h>


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


void normalize_buffer(char *s) {
    char *src = s;
    char *dst = s;
    bool in_space = true;

    while (*src) {
        unsigned char c = *src;

        // Check for whitespace or punctuation to convert to space
        // TODO: when tokenizing the strings, remove the trailing dot (.) and colon (:)
        if (c <= 32 || c == ',' || c == '!' || c == '?' || c == ';' || c == '(' || c == ')' ) {
            if (!in_space) {
                *dst++ = ' ';
                in_space = true;
            }
        } else {
            *dst++ = c;
            in_space = false;
        }

        src++;
    }

    // Remove trailing space
    if (dst > s && dst[-1] == ' ')
        dst--;

    *dst = '\0';
}


void chop_newlines(char *str, size_t len) {
    if (str == NULL) return;

    while (len > 0) {
        if (str[len - 1] == '\r' || str[len - 1] == '\n') {
            str[--len] = '\0';
        }
        else {
            break;
        }
    }
}


void extract_url_token(char *s, char *result, int resultlen) {
    memset(result, 0, resultlen);

    // Chop the request uri from the URL
    char *p = strchr(s, '/');
    if (p) *p = '\0';

    p = strrchr(s, '.');
    if (p) {
        *p = '\0';
        char *q = strrchr(s, '.');
        if (q) {
            *p = '.';
            p = q+1;
            *q = '\0';
        } else {
            *p = '.';
             p = s;
        }
    } else {
        p = s;
    }

    snprintf(result, resultlen-1, "URL*%s", p);
}


bool is_item_on_list(char *item, char *list) {
    if(!item || !list) return false;

    size_t itemlen = strlen(item);

    if (itemlen < 3 || strlen(list) < 3) return false;

    char *p = list;

    while (p) {
        char v[SMALLBUFSIZE];
        int result;
        p = split(p, ',', v, sizeof(v)-1, &result);

        size_t len = strlen(v);
        if (len < 3) continue;

        if (v[len-1] == '$') {
            v[len-1] = '\0'; // remove $
            size_t toklen = len - 1;
            if (itemlen >= toklen && strncasecmp(item + itemlen - toklen, v, toklen) == 0) {
                return true;
            }
        } else if (strcasestr(item, v)) {
            return true;
        }
   }

   return false;
}
