/*
 * buffer.h, 2008.01.20, SJ
 * copied from the dspam project
 */

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

