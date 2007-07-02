/*
 * hash.h, 2006.06.06, SJ
 */

#include "cfg.h"

#define MAXHASH 4099 /* if you want more try this value: 74713 */
#define MAX_HASH_STR_LEN 64

struct node {
   char str[MAX_HASH_STR_LEN];
   double spaminess;
   double deviation;
   unsigned long num;
   struct node *r;
};

void inithash(struct node *xhash[MAXHASH]);
float clearhash(struct node *xhash[MAXHASH]);
struct node *makenewnode(struct node *xhash[MAXHASH], char *s, double spaminess, double deviation);
int addnode(struct node *xhash[MAXHASH], char *s, double spaminess, double deviation);
struct node *findnode(struct node *xhash[MAXHASH], char *s);
double sorthash(struct node *xhash[MAXHASH], int top10, struct __config cfg);
unsigned long hash(char *s);
