#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

int decode_entity(const char *src, int *consumed, char *out) {
    // Simple entity decoder for common HTML entities
    struct {
        const char *entity;
        char ch;
    } entities[] = {
        {"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'},
        {"&quot;", '\"'}, {"&apos;", '\''}, {"&nbsp;", ' '},
    };
    for (size_t i = 0; i < sizeof(entities)/sizeof(entities[0]); i++) {
        int len = strlen(entities[i].entity);
        if (strncmp(src, entities[i].entity, len) == 0) {
            *out = entities[i].ch;
            *consumed = len;
            return 1;
        }
    }
    // Handle numeric entities like &#65; or &#x41;
    if (src[0] == '&' && src[1] == '#') {
        int val = 0, i = 2;
        if (src[i] == 'x' || src[i] == 'X') {
            i++;
            while (isxdigit(src[i])) {
                val = val * 16 + (isdigit(src[i]) ? src[i] - '0' : tolower(src[i]) - 'a' + 10);
                i++;
            }
        } else {
            while (isdigit(src[i])) {
                val = val * 10 + (src[i] - '0');
                i++;
            }
        }
        if (src[i] == ';') {
            *out = (char)(val);
            *consumed = i + 1;
            return 1;
        }
    }
    return 0;
}

void normalize_html(char *input) {
    char *src = input;
    char *dst = input;
    int in_tag = 0;
    int in_script = 0;
    int in_style = 0;

    while (*src) {
        if (!in_tag && *src == '<') {
            if (strncasecmp(src, "<script", 7) == 0) in_script = 1;
            else if (strncasecmp(src, "<style", 6) == 0) in_style = 1;
            in_tag = 1;
            src++;
        } else if (in_tag) {
            if (*src == '>') {
                in_tag = 0;
                if (in_script && strncasecmp(src - 8, "</script>", 9) == 0)
                    in_script = 0;
                else if (in_style && strncasecmp(src - 7, "</style>", 8) == 0)
                    in_style = 0;

                *dst++ = ' ';
            }
            src++;
        } else if (!in_script && !in_style) {
            if (*src == '&') {
                char decoded;
                int consumed;
                if (decode_entity(src, &consumed, &decoded)) {
                    *dst++ = decoded;
                    src += consumed;
                } else {
                    *dst++ = *src++;
                }
            } else {
                *dst++ = *src++;
            }
        } else {
            // Inside <script> or <style> content, skip until end tag
            if (in_script && strncasecmp(src, "</script>", 9) == 0) {
                in_script = 0;
                in_tag = 1;
                src += 1;  // Let the tag logic handle it
            } else if (in_style && strncasecmp(src, "</style>", 8) == 0) {
                in_style = 0;
                in_tag = 1;
                src += 1;  // Let the tag logic handle it
            } else {
                src++;
            }
        }
    }
    *dst = '\0';
}
