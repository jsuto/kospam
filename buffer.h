/*
 * buffer.h, 2008.04.05, SJ
 * copied from the dspam project
 */

#ifndef _BUFFER_H
 #define _BUFFER_H

typedef struct {
   long size;
   long used;
   char *data;
} buffer;

buffer *buffer_create(const char *);
void buffer_destroy(buffer *);

int buffer_copy(buffer *, const char *);
int buffer_cat(buffer *, const char *);
int buffer_clear(buffer *);

#endif /* _BUFFER_H */
