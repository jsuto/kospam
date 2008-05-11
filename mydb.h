/*
 * mydb.h, 2008.05.11, SJ
 */

#ifndef _MYDB_H
 #define _MYDB_H

#include "parser.h"

struct mydb {
   unsigned long long key;
   unsigned short int nham;
   unsigned short int nspam;
   unsigned long ts;
};

struct mydb_node {
   unsigned long long key;
   unsigned short int nham;
   unsigned short int nspam;
   unsigned long ts;
   unsigned int pos;
   struct mydb_node *r;
};

#define MYDB_HEADER_SIZE 8
#define N_SIZE 16
#define SEGMENT_SIZE 768
#define MAX_MYDB_HASH 74713

int init_mydb(char *mydb_file, struct mydb_node *xhash[MAX_MYDB_HASH], struct session_data *sdata);
void close_mydb(struct mydb_node *xhash[MAX_MYDB_HASH]);
unsigned long long mydb_hash(unsigned long long key);
struct mydb_node *makenewmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
int addmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
struct mydb_node *findmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key);
int add_or_update(int fd, struct mydb_node *mhash[MAX_MYDB_HASH], int ham_or_spam, char *token, int train_mode, unsigned long ts);
int update_tokens(char *mydbfile, struct mydb_node *xhash[MAX_MYDB_HASH], struct _token *token);

float mydbqry(struct mydb_node *Mhash[MAX_MYDB_HASH], char *p, struct session_data *sdata, float rob_s, float rob_x);
int my_walk_hash(char *mydbfile, struct mydb_node *xhash[MAX_MYDB_HASH], int ham_or_spam, struct _token *token, int train_mode);
int update_tokens(char *mydbfile, struct mydb_node *xhash[MAX_MYDB_HASH], struct _token *token);

#endif /* _MYDB_H */
