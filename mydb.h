/*
 * mydb.h, 2008.01.23, SJ
 */

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

float Nham, Nspam;

int init_mydb(char *mydb_file, struct mydb_node *xhash[MAX_MYDB_HASH]);
void close_mydb(struct mydb_node *xhash[MAX_MYDB_HASH]);
unsigned long long mydb_hash(unsigned long long key);
struct mydb_node *makenewmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
int addmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
struct mydb_node *findmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key);
int add_or_update(int fd, struct mydb_node *mhash[MAX_MYDB_HASH], int ham_or_spam, char *token, int train_mode, unsigned long ts);
int update_tokens(char *mydbfile, struct mydb_node *xhash[MAX_MYDB_HASH], struct _token *token);
