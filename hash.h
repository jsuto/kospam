/*
 * hash.h, 2009.01.15, SJ
 */

#ifndef _HASH_H
 #define _HASH_H

#include "cfg.h"
#include "defs.h"


void inithash(struct node *xhash[]);
void clearhash(struct node *xhash[], int print);
int counthash(struct node *xhash[]);
struct node *makenewnode(struct node *xhash[], char *s, double spaminess, double deviation);
int addnode(struct node *xhash[], char *s, double spaminess, double deviation);
struct node *findnode(struct node *xhash[], char *s);
int updatenode(struct node *xhash[], unsigned long long key, float nham, float nspam, float spaminess, float deviation);
void calcnode(struct node *xhash[], float Nham, float Nspam, struct __config *cfg);
inline int hash(unsigned long long key);

#endif /* _HASH_H */
