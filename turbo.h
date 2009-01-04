/*
 * turbo.h, 2009.01.03, SJ
 */

#ifndef _TURBO_H
 #define _TURBO_H

#include "cfg.h"
#include "buffer.h"

/* other primes you can use: 200003, 400051, 500177 */
#define MAX_TURBO_HASH 100003
#define MAX_TOKENS_TO_UPDATE_IN_1_ROUND 8192

struct turbo {
   unsigned long long key;
   float spaminess;
   unsigned char dirty;
   struct turbo *r;
};

void initturbo(struct turbo *xhash[]);
void clearturbo(struct turbo *xhash[]);
struct turbo *newturbo(struct turbo *xhash[], unsigned long long key, float spaminess);
int addturbo(struct turbo *xhash[], unsigned long long key, float spaminess);
float spamturbo(struct turbo *xhash[], unsigned long long key);
inline unsigned long hashturbo(unsigned long long key);
int flush_dirty_turbo(struct turbo *xhash[], buffer *query);

#endif /* _TURBO_H */
