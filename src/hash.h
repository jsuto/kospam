/*
 * hash.h, SJ
 */

#ifndef _HASH_H
 #define _HASH_H

#include "cfg.h"
#include "defs.h"


void inithash(struct node *xhash[]);
void clearhash(struct node *xhash[]);
void resetcounters(struct node *xhash[]);
void printhash(struct node *xhash[]);
struct node *makenewnode(char *s, double spaminess, double deviation);
int addnode(struct node *xhash[], char *s, double spaminess, double deviation);
struct node *findnode(struct node *xhash[], char *s);
int is_substr_in_hash(struct node *xhash[], char *s);
int updatenode(struct node *xhash[], uint64 key, float nham, float nspam, float spaminess, float deviation);

#endif /* _HASH_H */
