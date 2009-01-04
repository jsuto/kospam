/*
 * cache.h, 2009.01.04, SJ
 */

#ifndef _CACHE_H
 #define _CACHE_H

#include "cfg.h"
#include "buffer.h"

/* other primes you can use: 200003, 400051, 500177 */
#define MAX_CACHE_HASH 100003
#define MAX_TOKENS_TO_UPDATE_IN_1_ROUND 8192

#define NETWORK_SEGMENT_SIZE 1000
#define PKT_SIZE sizeof(struct x_token) * NETWORK_SEGMENT_SIZE
#define MY_PORT 29781

struct cache {
   unsigned long long key;
   float spaminess;
   unsigned char dirty;
   struct cache *r;
};

struct x_token {
   unsigned long long token;
   float spaminess;
};

void initcache(struct cache *xhash[]);
void clearcache(struct cache *xhash[]);
struct cache *newcache(struct cache *xhash[], unsigned long long key, float spaminess);
int addcache(struct cache *xhash[], unsigned long long key, float spaminess);
float spamcache(struct cache *xhash[], unsigned long long key);
inline unsigned long hashcache(unsigned long long key);
int flush_dirty_cache(struct cache *xhash[], buffer *query);

#endif /* _CACHE_H */
