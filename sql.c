/*
 * sql.c, 2007.07.16, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <sys/mman.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "sql.h"
#include "config.h"


#ifdef HAVE_MYSQL_TOKEN_DATABASE
   #include <mysql.h>
   int do_mysql_qry(MYSQL mysql, int sockfd, int ham_or_spam, char *token, char *tokentable, unsigned int uid, int train_mode);
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   int do_sqlite3_qry(sqlite3 *db, int sockfd, int ham_or_spam, char *token, char *tokentable, unsigned int uid, int train_mode);
#endif

/*
 * query the spamicity value of a token from token database
 */

float SQL_QUERY(qry QRY, char *tokentable, char *token, struct node *xhash[MAXHASH]){
   float r = DEFAULT_SPAMICITY, ham_prob=0, spam_prob=0;
   int n;
   struct te TE;

   TE.nham = TE.nspam = 0;

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   TE = myqry(QRY.mysql, QRY.sockfd, tokentable, token, QRY.uid);
#endif
#ifdef HAVE_SQLITE3
   TE = sqlite3_qry(QRY.db, QRY.sockfd, tokentable, token, QRY.uid);
#endif

   if(TE.nham == 0 && TE.nspam == 0) return r;

   /* add token to list if not mature enough, 2007.07.09, SJ */

   if(QRY.uid > 0 && TE.nham < TUM_LIMIT && TE.nspam < TUM_LIMIT)
      addnode(xhash, token, 0 , 0);


   /* if the token has occurred at least N times, 2007.06.13, SJ */

   if(TE.nham + TE.nspam > 1){
      ham_prob = TE.nham / QRY.ham_msg;
      spam_prob = TE.nspam / QRY.spam_msg;

      if(ham_prob > 1) ham_prob = 1;
      if(spam_prob > 1) spam_prob = 1;

      r = spam_prob / (ham_prob + spam_prob);

      /* deal with rare words */

      if(TE.nham < FREQ_MIN && TE.nspam < FREQ_MIN){
         n = TE.nham;
         if(TE.nspam > n) n = TE.nspam;

         r = (0.5 + n * r) / (1+n);
      }

   }

   if(r < REAL_HAM_TOKEN_PROBABILITY) r = REAL_HAM_TOKEN_PROBABILITY;
   if(r > REAL_SPAM_TOKEN_PROBABILITY) r = REAL_SPAM_TOKEN_PROBABILITY;

   //fprintf(stderr, "token: %s=%.4f\n", token, r);

   return r;
}


/*
 * walk through the hash table and add/update its elements in mysql table
 */

int my_walk_hash(qry QRY, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], int train_mode){
   int i, n=0;
   struct node *p, *q;

   for(i=0;i<MAXHASH;i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

      #ifdef HAVE_MYSQL_TOKEN_DATABASE
         do_mysql_qry(QRY.mysql, QRY.sockfd, ham_or_spam, p->str, tokentable, QRY.uid, train_mode);
      #endif
      #ifdef HAVE_SQLITE3
         do_sqlite3_qry(QRY.db, QRY.sockfd, ham_or_spam, p->str, tokentable, QRY.uid, train_mode);
      #endif

         n++;

         q = q->r;
         if(p)
            free(p);
      }
      xhash[i] = NULL;
   }

   return n;
}

