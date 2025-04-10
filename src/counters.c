/*
 * counters.c, SJ
 */

#include <kospam.h>


struct __counters load_counters(MYSQL *conn){
   char buf[SMALLBUFSIZE];
   struct __counters counters;
   struct query sql;

   bzero(&counters, sizeof(counters));

   snprintf(buf, sizeof(buf)-1, "SELECT `rcvd`, `size`, `ham`, `spam`, `possible_spam`, `unsure`, `minefield`, `virus`, `fp`, `fn`, `zombie`, `mynetwork` FROM `%s`", SQL_COUNTER_TABLE);

   if(prepare_sql_statement(conn, &sql, buf) == ERR) return counters;


   p_bind_init(&sql);

   if(p_exec_stmt(conn, &sql) == OK){

      p_bind_init(&sql);

      sql.sql[sql.pos] = (char *)&counters.c_rcvd; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_size; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_ham; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_spam; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_possible_spam; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_unsure; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_minefield; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_virus; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_fp; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_fn; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_zombie; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;
      sql.sql[sql.pos] = (char *)&counters.c_mynetwork; sql.type[sql.pos] = TYPE_LONGLONG; sql.len[sql.pos] = sizeof(uint64); sql.pos++;

      p_store_results(&sql);
      p_fetch_results(&sql);
      p_free_results(&sql);
   }

   close_prepared_statement(&sql);

   return counters;
}


void update_counters(MYSQL *conn, struct __counters *counters){
   char buf[MAXBUFSIZE];
#ifdef HAVE_MEMCACHED
   unsigned long long mc, rcvd;
   struct __counters c;
   char key[MAX_MEMCACHED_KEY_LEN];
   unsigned int flags=0;

   if(cfg->update_counters_to_memcached == 1){

      /* increment counters to memcached */

      if(memcached_increment(&(data->memc), MEMCACHED_MSGS_RCVD, strlen(MEMCACHED_MSGS_RCVD), counters->c_rcvd, &mc) == MEMCACHED_SUCCESS){
         rcvd = mc;

         if(counters->c_size > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_SIZE, strlen(MEMCACHED_MSGS_SIZE), counters->c_size, &mc);
         if(counters->c_ham > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_HAM, strlen(MEMCACHED_MSGS_HAM), counters->c_ham, &mc);
         if(counters->c_spam > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_SPAM, strlen(MEMCACHED_MSGS_SPAM), counters->c_spam, &mc);
         if(counters->c_possible_spam > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_POSSIBLE_SPAM, strlen(MEMCACHED_MSGS_POSSIBLE_SPAM), counters->c_possible_spam, &mc);
         if(counters->c_unsure > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_UNSURE, strlen(MEMCACHED_MSGS_UNSURE), counters->c_unsure, &mc);
         if(counters->c_minefield > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_MINEFIELD, strlen(MEMCACHED_MSGS_MINEFIELD), counters->c_minefield, &mc);
         if(counters->c_virus > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_VIRUS, strlen(MEMCACHED_MSGS_VIRUS), counters->c_virus, &mc);
         if(counters->c_zombie > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_ZOMBIE, strlen(MEMCACHED_MSGS_ZOMBIE), counters->c_zombie, &mc);
         if(counters->c_fp > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_FP, strlen(MEMCACHED_MSGS_FP), counters->c_fp, &mc);
         if(counters->c_fn > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_FN, strlen(MEMCACHED_MSGS_FN), counters->c_fn, &mc);
         if(counters->c_mynetwork > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_MYNETWORK, strlen(MEMCACHED_MSGS_MYNETWORK), counters->c_mynetwork, &mc);

         bzero(&c, sizeof(c));

         snprintf(buf, sizeof(buf)-1, "%s %s %s %s %s %s %s %s %s %s %s %s %s", MEMCACHED_MSGS_RCVD, MEMCACHED_MSGS_SIZE, MEMCACHED_MSGS_MYNETWORK, MEMCACHED_MSGS_HAM, MEMCACHED_MSGS_SPAM, MEMCACHED_MSGS_POSSIBLE_SPAM, MEMCACHED_MSGS_UNSURE, MEMCACHED_MSGS_MINEFIELD, MEMCACHED_MSGS_VIRUS, MEMCACHED_MSGS_ZOMBIE, MEMCACHED_MSGS_FP, MEMCACHED_MSGS_FN, MEMCACHED_COUNTERS_LAST_UPDATE);

         if(memcached_mget(&(data->memc), buf) == MEMCACHED_SUCCESS){
            while((memcached_fetch_result(&(data->memc), &key[0], &buf[0], &flags))){
               if(!strcmp(key, MEMCACHED_MSGS_RCVD)) c.c_rcvd = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_MYNETWORK)) c.c_mynetwork = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_SIZE)) c.c_size = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_HAM)) c.c_ham = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_SPAM)) c.c_spam = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_POSSIBLE_SPAM)) c.c_possible_spam = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_UNSURE)) c.c_unsure = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_MINEFIELD)) c.c_minefield = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_VIRUS)) c.c_virus = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_ZOMBIE)) c.c_zombie = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_FP)) c.c_fp = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_FN)) c.c_fn = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_COUNTERS_LAST_UPDATE)) mc = strtoull(buf, NULL, 10);
            }


            if(sdata->now - mc > cfg->memcached_to_db_interval && c.c_rcvd > 0 && c.c_rcvd >= rcvd){
               snprintf(buf, sizeof(buf)-1, "%ld", sdata->now); memcached_set(&(data->memc), MEMCACHED_COUNTERS_LAST_UPDATE, strlen(MEMCACHED_COUNTERS_LAST_UPDATE), buf, strlen(buf), 0, 0);

               snprintf(buf, sizeof(buf)-1, "UPDATE %s SET rcvd=%llu, size=%llu, mynetwork=%llu, ham=%llu, spam=%llu, possible_spam=%llu, unsure=%llu, minefield=%llu, virus=%llu, zombie=%llu, fp=%llu, fn=%llu", SQL_COUNTER_TABLE, c.c_rcvd, c.c_size, c.c_mynetwork, c.c_ham, c.c_spam, c.c_possible_spam, c.c_unsure, c.c_minefield, c.c_virus, c.c_zombie, c.c_fp, c.c_fn);

               p_query(sdata, buf);
            }
         }

      }
      else {

         c = load_counters(sdata);

         snprintf(buf, sizeof(buf)-1, "%ld", sdata->now); memcached_add(&(data->memc), MEMCACHED_COUNTERS_LAST_UPDATE, strlen(MEMCACHED_COUNTERS_LAST_UPDATE), buf, strlen(buf), 0, 0);

         snprintf(buf, sizeof(buf)-1, "%llu", c.c_ham + counters->c_ham); memcached_add(&(data->memc), MEMCACHED_MSGS_HAM, strlen(MEMCACHED_MSGS_HAM), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_spam + counters->c_spam); memcached_add(&(data->memc), MEMCACHED_MSGS_SPAM, strlen(MEMCACHED_MSGS_SPAM), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_possible_spam + counters->c_possible_spam); memcached_add(&(data->memc), MEMCACHED_MSGS_POSSIBLE_SPAM, strlen(MEMCACHED_MSGS_POSSIBLE_SPAM), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_unsure + counters->c_unsure); memcached_add(&(data->memc), MEMCACHED_MSGS_UNSURE, strlen(MEMCACHED_MSGS_UNSURE), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_minefield + counters->c_minefield); memcached_add(&(data->memc), MEMCACHED_MSGS_MINEFIELD, strlen(MEMCACHED_MSGS_MINEFIELD), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_virus + counters->c_virus); memcached_add(&(data->memc), MEMCACHED_MSGS_VIRUS, strlen(MEMCACHED_MSGS_VIRUS), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_zombie + counters->c_zombie); memcached_add(&(data->memc), MEMCACHED_MSGS_ZOMBIE, strlen(MEMCACHED_MSGS_ZOMBIE), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_fp + counters->c_fp); memcached_add(&(data->memc), MEMCACHED_MSGS_FP, strlen(MEMCACHED_MSGS_FP), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_fn + counters->c_fn); memcached_add(&(data->memc), MEMCACHED_MSGS_FN, strlen(MEMCACHED_MSGS_FN), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_rcvd + counters->c_rcvd); memcached_add(&(data->memc), MEMCACHED_MSGS_RCVD, strlen(MEMCACHED_MSGS_RCVD), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_mynetwork + counters->c_mynetwork); memcached_add(&(data->memc), MEMCACHED_MSGS_MYNETWORK, strlen(MEMCACHED_MSGS_MYNETWORK), buf, strlen(buf), 0, 0);
         snprintf(buf, sizeof(buf)-1, "%llu", c.c_size + counters->c_size); memcached_add(&(data->memc), MEMCACHED_MSGS_SIZE, strlen(MEMCACHED_MSGS_SIZE), buf, strlen(buf), 0, 0);
      }

   }
   else {
#endif
      snprintf(buf, sizeof(buf)-1, "UPDATE `%s` SET rcvd=rcvd+%llu, size=size+%llu, mynetwork=mynetwork+%llu, ham=ham+%llu, spam=spam+%llu, possible_spam=possible_spam+%llu, unsure=unsure+%llu, minefield=minefield+%llu, virus=virus+%llu, zombie=zombie+%llu, fp=fp+%llu, fn=fn+%llu", SQL_COUNTER_TABLE, counters->c_rcvd, counters->c_size, counters->c_mynetwork, counters->c_ham, counters->c_spam, counters->c_possible_spam, counters->c_unsure, counters->c_minefield, counters->c_virus, counters->c_zombie, counters->c_fp, counters->c_fn);

      p_query(conn, buf);

#ifdef HAVE_MEMCACHED
   }
#endif

}
