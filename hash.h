/*
 * hash.h, 2008.09.15, SJ
 */

#ifndef _HASH_H
 #define _HASH_H

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

void inithash(struct node *xhash[]);
void clearhash(struct node *xhash[]);
struct node *makenewnode(struct node *xhash[], char *s, double spaminess, double deviation);
int addnode(struct node *xhash[], char *s, double spaminess, double deviation);
struct node *findnode(struct node *xhash[], char *s);
unsigned long hash(char *s);

#endif /* _HASH_H */
