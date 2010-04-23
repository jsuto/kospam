#include <stdio.h>
#include <string.h>
#include <libmemcached/memcached.h>
#include <clapf.h>

int main(){
   memcached_server_st *servers;
   memcached_st *memc;
   memcached_result_st *s, *result;
   memcached_return_t rc;
  const char *keys[]= {MEMCACHED_MSGS_RCVD, MEMCACHED_MSGS_HAM, MEMCACHED_MSGS_SPAM, MEMCACHED_MSGS_POSSIBLE_SPAM, MEMCACHED_MSGS_UNSURE, MEMCACHED_MSGS_MINEFIELD, MEMCACHED_MSGS_VIRUS, MEMCACHED_MSGS_FP, MEMCACHED_MSGS_FN };
   size_t key_length[]= {strlen(MEMCACHED_MSGS_RCVD), strlen(MEMCACHED_MSGS_HAM), strlen(MEMCACHED_MSGS_SPAM), strlen(MEMCACHED_MSGS_POSSIBLE_SPAM), strlen(MEMCACHED_MSGS_UNSURE), strlen(MEMCACHED_MSGS_MINEFIELD), strlen(MEMCACHED_MSGS_VIRUS), strlen(MEMCACHED_MSGS_FP), strlen(MEMCACHED_MSGS_FN)};
   unsigned long long val;
   int x=0;

   memc = memcached_create(NULL);

   if(memc != NULL){
      servers = memcached_servers_parse("127.0.0.1");

      if(memcached_server_push(memc, servers) != MEMCACHED_SUCCESS){
         memcached_free(memc);
         memc = NULL;
      }

      memcached_server_list_free(servers);
   }


   rc = memcached_mget(memc, keys, key_length, 9);


   while((s = memcached_fetch_result(memc, NULL, &rc))){
      val = strtoull(s->value.string, NULL, 10);
      printf("from mcache: %s/%llu\n", s->value.string, val);
      free(s);
   }

   memcached_free(memc);

   return 0;
}
