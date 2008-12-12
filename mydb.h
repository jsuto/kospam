/*
 * mydb.h, 2008.12.12, SJ
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

struct x_token {
   unsigned long long token;
   float spaminess;
};

#define MYDB_HEADER_SIZE 8
#define N_SIZE 16
#define SEGMENT_SIZE 768
#define MAX_MYDB_HASH 74713

#define NETWORK_SEGMENT_SIZE 1000
#define PKT_SIZE sizeof(struct x_token) * NETWORK_SEGMENT_SIZE
#define MY_PORT 29781

void init_my_hash(struct mydb_node *xhash[]);
int init_mydb(char *mydb_file, struct mydb_node *xhash[], struct session_data *sdata);
void close_mydb(struct mydb_node *xhash[]);
unsigned long long mydb_hash(unsigned long long key);
struct mydb_node *makenewmydb_node(struct mydb_node *xhash[], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
int addmydb_node(struct mydb_node *xhash[], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
struct mydb_node *findmydb_node(struct mydb_node *xhash[], unsigned long long key);
int add_or_update(int fd, struct mydb_node *mhash[], int ham_or_spam, char *token, int train_mode, unsigned long ts);
int update_tokens(char *mydbfile, struct mydb_node *xhash[], struct _token *token);

float mydbqry(struct mydb_node *Mhash[], char *p, struct session_data *sdata, float rob_s, float rob_x);
int my_walk_hash(char *mydbfile, struct mydb_node *xhash[], int ham_or_spam, struct _token *token, int train_mode);
int update_tokens(char *mydbfile, struct mydb_node *xhash[], struct _token *token);

void hash_2_to_1(struct mydb_node *xhash[], struct mydb_node *xhash2[], struct mydb_node *xhash3[]);

double x_spam_check(struct session_data *sdata, struct _state *state, struct __config cfg);

#endif /* _MYDB_H */
