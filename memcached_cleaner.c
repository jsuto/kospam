/*
 * memcached_cleaner.c, 2009.09.18, SJ
 */

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <mysql.h>
#include <libmemcached/memcached.h>
#include <unistd.h>
#include <clapf.h>
#include "config.h"

#define TOKENS_TO_FLUSH 2048

extern char *optarg;
extern int optind;
int queries = 0, total_time=0;

memcached_st *memc2;
MYSQL mysql;

struct node *uhash[MAXHASH];

struct timezone tz;
struct timeval tv1, tv2;



int run_sql_query(struct node *xhash[]){
   unsigned long now;
   time_t cclock;
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *p, *q;
   buffer *query;

   query = buffer_create(NULL);
   if(!query) return -1;

   time(&cclock);
   now = cclock;

   snprintf(s, SMALLBUFSIZE-1, "UPDATE %s SET timestamp=%ld WHERE token in (0", SQL_TOKEN_TABLE, now);

   buffer_cat(query, s);


   for(i=0; i<MAXHASH; i++){
      q = xhash[i];
      while(q != NULL){
         p = q;

         snprintf(s, SMALLBUFSIZE-1, ",%llu", p->key);
         buffer_cat(query, s);
         n++;

         q = q->r;
      }
   }

   buffer_cat(query, ")");

   mysql_real_query(&mysql, query->data, strlen(query->data));

   buffer_destroy(query);

   return n;
}


int parse_entries(const char *key, size_t key_length){
   char *q, *query, u[BUFLEN];
   size_t len;
   uint32_t flags;
   memcached_return rc;

   if(strncmp(key, MEMCACHED_MESSAGE_NAME, strlen(MEMCACHED_MESSAGE_NAME)) == 0){
      query = memcached_get(memc2, key, key_length, &len, &flags, &rc);
      //printf(".");

      q = query;

      do {
         q = split_str(q, ",", u, BUFLEN-1);
         addnode(uhash, u, 0.99, 0.49);

         if(counthash(uhash) > TOKENS_TO_FLUSH) {
            gettimeofday(&tv1, &tz);

            run_sql_query(uhash);

            gettimeofday(&tv2, &tz);

            total_time += tvdiff(tv2, tv1)/1000;

            //printf("sql qry in %ld ms\n", tvdiff(tv2, tv1)/1000);

            clearhash(uhash, 0);
         }

      } while(q);

      memcached_delete(memc2, key, key_length, 0);

      queries++;
   }

   return 1;
}


static memcached_return key_printer(memcached_st *ptr __attribute__((unused)),  
                                              const char *key, size_t key_length, 
                                              void *context __attribute__((unused))) {

   parse_entries(key, key_length);

   return MEMCACHED_SUCCESS;
}


int main(int argc, char **argv){
   int i, rc=1, uid=0, gid=0;
   static int opt_binary=0;
   char *configfile = CONFIG_FILE;
   struct __config cfg;
   memcached_st *memc;
   memcached_server_st *servers;
   //memcached_return rec;
   memcached_dump_func callbacks[1];

   callbacks[0] = &key_printer;


   while((i = getopt(argc, argv, "c:u:g:")) > 0){
      switch(i){

        case 'c' :
                    configfile = optarg;
                    break;

        case 'u' :
                    uid = atoi(optarg);
                    break;

        case 'g' :
                    gid = atoi(optarg);
                    break;

        default  : 
                    __fatal(CLAPFUSAGE);
      }
   }


   (void) openlog("memcached_cleaner", LOG_PID, LOG_MAIL);

   cfg = read_config(configfile);

   inithash(uhash);

   mysql_init(&mysql);
   mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
   mysql_options(&mysql, MYSQL_OPT_RECONNECT, (const char*)&rc);


   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0) == 0){
      syslog(LOG_PRIORITY, "cannot connect to mysql server");
      return -1;
   }

 
   memc = memcached_create(NULL);
   if(!memc){
      syslog(LOG_PRIORITY, "cannot create memcached structure");
      return -1;
   }

   servers = memcached_servers_parse(cfg.memcached_servers);


   if(memcached_server_push(memc, servers) != MEMCACHED_SUCCESS){
      syslog(LOG_PRIORITY, "memcached_server_push()\n");
      return -1;
   }

   memcached_server_list_free(servers);


   memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, (uint64_t)opt_binary);
   memc2 = memcached_clone(NULL, memc);


   memcached_dump(memc, callbacks, NULL, 1);



   memcached_free(memc);
   memcached_free(memc2);


   if(counthash(uhash) > 0) {
      //printf("last sql qry\n");

      run_sql_query(uhash);
      clearhash(uhash, 0);
   }

   mysql_close(&mysql);


   //printf("freed %d entries from memcached in %d ms\n", queries, total_time);

   syslog(LOG_PRIORITY, "freed %d entries from memcached in %d ms", queries, total_time);

   return 0;
}

