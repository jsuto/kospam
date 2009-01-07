/*
 * mydb.h, 2008.12.12, SJ
 */

#ifndef _MYDB_H
 #define _MYDB_H

#include "defs.h"
#include "parser.h"

void init_my_hash(struct mydb_node *xhash[]);
int init_mydb(char *mydb_file, struct mydb_node *xhash[], struct session_data *sdata);
void close_mydb(struct mydb_node *xhash[]);
inline unsigned long long mydb_hash(unsigned long long key);
struct mydb_node *makenewmydb_node(struct mydb_node *xhash[], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
int addmydb_node(struct mydb_node *xhash[], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
struct mydb_node *findmydb_node(struct mydb_node *xhash[], unsigned long long key);
int add_or_update(int fd, struct mydb_node *mhash[], int ham_or_spam, char *token, int train_mode, unsigned long ts);

float mydbqry(struct session_data *sdata, char *p, struct __config *cfg);
int my_walk_hash(char *mydbfile, struct mydb_node *xhash[], int ham_or_spam, struct node *thash[], int train_mode);
int update_tokens(char *mydbfile, struct mydb_node *xhash[], struct node *thash[]);

void hash_2_to_1(struct mydb_node *xhash[], struct mydb_node *xhash2[], struct mydb_node *xhash3[]);

#endif /* _MYDB_H */
