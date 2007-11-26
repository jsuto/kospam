/*
 * mydb.c, 2007.11.26, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>
#include "misc.h"
#include "messages.h"
#include "parser.h"
#include "hash.h"
#include "mydb.h"
#include "config.h"


unsigned long long mydb_hash(unsigned long long key);
struct mydb_node *makenewmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
int addmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos);
struct mydb_node *findmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key);


struct te {
   unsigned int nham;
   unsigned int nspam;
};

int init_mydb(char *mydb_file, struct mydb_node *xhash[MAX_MYDB_HASH]){
   int i, n, fd, pos;
   unsigned char buf[N_SIZE*SEGMENT_SIZE];
   unsigned long x;
   struct mydb e;

   pos = Nham = Nspam = 0;

   for(i=0;i<MAX_MYDB_HASH;i++)
      xhash[i] = NULL;

   fd = open(mydb_file, O_RDONLY);
   if(fd == -1){
      /* try to create database */
      fd = open(mydb_file, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
      if(fd != -1){
         x = 0;
         write(fd, &x, 4);
         write(fd, &x, 4);
         close(fd);

         return 1;
      }
      return 0;
   }

   read(fd, &x, 4);
   Nham = (float)x;
   read(fd, &x, 4);
   Nspam = (float)x;

   while((n = read(fd, buf, N_SIZE*SEGMENT_SIZE))){
      for(i=0; i<n/N_SIZE; i++){
         memcpy(&e, &buf[N_SIZE*i], N_SIZE);
         addmydb_node(xhash, e.key, e.nham, e.nspam, e.ts, pos);
         pos++;
      }
   }

   close(fd);

   return 1;
}


void close_mydb(struct mydb_node *xhash[MAX_MYDB_HASH]){
   int i;
   struct mydb_node *p, *q;

   for(i=0;i<MAX_MYDB_HASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;
         q = q->r;
         if(p)
            free(p);
      }
      xhash[i] = NULL;
   }
}


unsigned long long mydb_hash(unsigned long long key){
    return key % MAX_MYDB_HASH;
}

struct mydb_node *makenewmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos){
   struct mydb_node *h;

   if((h = malloc(sizeof(struct mydb_node))) == NULL)
      return NULL;

   memset(h, 0, sizeof(struct mydb_node));

   h->key = key;
   h->nham = nham;
   h->nspam = nspam;
   h->ts = ts;
   h->pos = pos;
   h->r = NULL;

   return h;
}


int addmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key, unsigned int nham, unsigned int nspam, unsigned long ts, unsigned int pos){
   struct mydb_node *p=NULL, *q;

   if(xhash[mydb_hash(key)] == NULL){
      xhash[mydb_hash(key)] = makenewmydb_node(xhash, key, nham, nspam, ts, pos);
   }
   else {
      q = xhash[mydb_hash(key)];
      while(q != NULL){
         p = q;
         if(q->key == key){
            q->nham += nham;
            q->nspam += nspam;
            q->pos = pos;
            return 0;
         }
         else
            q = q->r;
      }
      p->r = makenewmydb_node(xhash, key, nham, nspam, ts, pos);
   }

   return 1;
}


struct mydb_node *findmydb_node(struct mydb_node *xhash[MAX_MYDB_HASH], unsigned long long key){
   struct mydb_node *p, *q;

   if(key == 0)
      return NULL;

   q = xhash[mydb_hash(key)];

   if(q == NULL)
      return NULL;

   while(q != NULL){
      p = q;
      if(q->key == key){
         return q;
      }
      else
         q = q->r;
   }

   return NULL;
}


float mydbqry(struct mydb_node *xhash[MAX_MYDB_HASH], char *p, float rob_s, float rob_x, struct node *qhash[MAXHASH]){
   struct mydb_node *q;
   unsigned long long key;
   float spamicity = DEFAULT_SPAMICITY;
   int freq_min = FREQ_MIN;

   if(p == NULL) return spamicity;

   key = APHash(p);

   q = findmydb_node(xhash, key);
   if(q == NULL) return spamicity;

   if(q->nham < TUM_LIMIT && q->nspam < TUM_LIMIT)
      addnode(qhash, p, 0 , 0);

   if(!strchr(p, '+')) freq_min = 2*FREQ_MIN;

   spamicity = calc_spamicity(Nham, Nspam, q->nham, q->nspam, rob_s, rob_x, freq_min);

   return spamicity;
}


int add_or_update(int fd, int ham_or_spam, char *token, int train_mode, unsigned long ts){
   unsigned long long key = APHash(token);
   struct mydb_node *q;
   struct mydb e;
   int n=0;

   if(fd == -1) return 0;

   q = findmydb_node(mhash, key);
   if(q){
      q->ts = ts;

      if(ham_or_spam == 1){
         q->nspam++;
         if(train_mode == T_TUM && q->nham > 0) q->nham--;
      }
      else {
         q->nham++;
         if(train_mode == T_TUM && q->nspam > 0) q->nspam--;
      }

      lseek(fd, MYDB_HEADER_SIZE + q->pos * N_SIZE, SEEK_SET);
      n = write(fd, q, N_SIZE);
   }

   /* not exists ... */

   else {
      e.key = key;
      e.ts = ts;

      if(ham_or_spam == 1){
         e.nham = 0;
         e.nspam = 1;
      }
      else {
         e.nham = 1;
         e.nspam = 0;
      }

      lseek(fd, 0, SEEK_END);
      n = write(fd, &e, N_SIZE);
   }

   return 0;
}


int my_walk_hash(char *mydbfile, struct mydb_node *xhash[MAX_MYDB_HASH], int ham_or_spam, struct node *qhash[MAXHASH], int train_mode){
   int i, fd, n=0;
   struct node *p, *q;
   unsigned long now, x;
   time_t cclock;

   time(&cclock);
   now = cclock;

   fd = open(mydbfile, O_RDWR);
   if(fd != -1){
      if(flock(fd, LOCK_EX|LOCK_NB)){
         close(fd);
         fd = -1;
      }
   }

   for(i=0;i<MAXHASH;i++){
      q = qhash[i];

      while(q != NULL){
         p = q;

         if(fd != 1){
            add_or_update(fd, ham_or_spam, p->str, train_mode, now);
         }

         n++;

         q = q->r;
         if(p)
            free(p);
      }
      qhash[i] = NULL;

   }

   if(fd != -1){

      if(ham_or_spam == 1){
         lseek(fd, 4, SEEK_SET);
      }
      else {
         lseek(fd, 0, SEEK_SET);
      }

      read(fd, &x, 4);
      x++;

      if(ham_or_spam == 1){
         lseek(fd, 4, SEEK_SET);
      }
      else {
         lseek(fd, 0, SEEK_SET);
      }

      write(fd, &x, 4);

      /* if it was a TUM request, then decrement the opposite counter, 2007.11.23, SJ */

      if(train_mode == T_TUM){
         if(ham_or_spam == 1)
            lseek(fd, 0, SEEK_SET);
         else
            lseek(fd, 4, SEEK_SET);

         read(fd, &x, 4);

         if(ham_or_spam == 1)
            lseek(fd, 0, SEEK_SET);
         else
            lseek(fd, 4, SEEK_SET);

         if(x > 1){
            x--;
            write(fd, &x, 4);
         }
      }

      flock(fd, LOCK_UN|LOCK_NB);
      close(fd);
   }

   return n;
}


int update_tokens(char *mydbfile, struct mydb_node *xhash[MAX_MYDB_HASH], struct node *qhash[MAXHASH]){
   int i, n, fd;
   struct node *q;
   struct mydb_node *Q;
   unsigned long now;
   unsigned long long key;
   time_t cclock;

   time(&cclock);
   now = cclock;

   fd = open(mydbfile, O_RDWR);
   if(fd == -1){
      syslog(LOG_PRIORITY, "cannot open mydb file %s", mydbfile);
      return 0;
   }

   if(flock(fd, LOCK_EX|LOCK_NB)){
      close(fd);
      syslog(LOG_PRIORITY, "cannot lock %s", mydbfile);
      return 0;
   }

   n = 0;

   for(i=0; i<MAXHASH; i++){
      q = qhash[i];

      while(q != NULL){
         if(fd != 1){
            key = APHash(q->str);

            Q = findmydb_node(xhash, key);
            if(Q){
               //fprintf(stderr, "updating timestamp of %s %llu\n", q->str, key);
               lseek(fd, MYDB_HEADER_SIZE + (Q->pos * N_SIZE) + 12, SEEK_SET);
               write(fd, &now, 4);
               n++;
            }
         }

         q = q->r;
      }
   }

   flock(fd, LOCK_UN|LOCK_NB);
   close(fd);

   return n;
}
