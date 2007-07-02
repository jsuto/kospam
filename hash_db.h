/*
 * hash_db.h, 2006.08.25, SJ
 */

#include "config.h"

struct t_node {
   char *str;
   float spaminess;
   struct t_node *r;
};

struct t_node *t_hash[MAX_TOKEN_HASH];

void init_token_hash(struct t_node *xhash[MAX_TOKEN_HASH]);
void clear_token_hash(struct t_node *xhash[MAX_TOKEN_HASH]);
struct t_node *makenewtoken(struct t_node *xhash[MAX_TOKEN_HASH], char *s, float spaminess);
int addtoken(struct t_node *xhash[MAX_TOKEN_HASH], char *s, float spaminess);
float get_spamicity(struct t_node *xhash[MAX_TOKEN_HASH], char *s);
unsigned long hash_token(char *s);
int read_datafile(char *filename, struct t_node *xhash[MAX_TOKEN_HASH]);
