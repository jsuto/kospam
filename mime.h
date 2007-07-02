/*
 * mime.h, 2006.01.27, SJ
 */

#include "config.h"

struct rfc822_attachment {
   char tmpdir[SMALLBUFSIZE];
   unsigned long cnt;
   int result;
};

struct rfc822_attachment extract_from_rfc822(char *message);

