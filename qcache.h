/*
 * qcache.h, 2007.10.26, SJ
 */

/* prime numbers: 32099, 65831, 128257 158209 192601 199999 */
#define MAXHASH 128257
#define MAX_ENTRIES_PER_SLOT 5

#define BANNER_QCACHE_220 "220 QCache OK\r\n"
#define BANNER_QCACHE_450 "450 QCache temporary error\r\n"
#define QCACHE_RESP_ERR "451 0 0\r\n"
#define QCACHE_RESP_OK "220 OK\r\n"

struct qcache {
   unsigned long long token;
   unsigned int uid;
   unsigned int nham;
   unsigned int nspam;
   unsigned long timestamp;
   unsigned char dirty;
   struct qcache *r;
};

struct token_entry {
   unsigned int nham;
   unsigned int nspam;
   unsigned char hit;
};


void inithash(struct qcache *xhash[MAXHASH]);
void clearhash(struct qcache *xhash[MAXHASH]);
struct qcache *makenewnode(struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid, unsigned int nham, unsigned int nspam, unsigned long timestamp);
int addnode(struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid, unsigned int nham, unsigned int nspam, unsigned long timestamp);
struct qcache *findnode(struct qcache *xhash[MAXHASH], unsigned long long token, unsigned int uid);
unsigned long hash(unsigned long long token);

