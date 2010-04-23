/*
 * counters.c, 2010.04.23, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <clapf.h>


struct __counters load_counters(struct session_data *sdata, struct __config *cfg){
   char buf[SMALLBUFSIZE];
   struct __counters counters;

   bzero(&counters, sizeof(counters));

   snprintf(buf, SMALLBUFSIZE-1, "SELECT rcvd, ham, spam, possible_spam, unsure, minefield, virus, fp, fn FROM t_counters");

#ifdef HAVE_MYSQL
   MYSQL_RES *res;
   MYSQL_ROW row;

   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            counters.c_rcvd = strtoull(row[0], NULL, 10);
            counters.c_ham = strtoull(row[1], NULL, 10);
            counters.c_spam = strtoull(row[2], NULL, 10);
            counters.c_possible_spam = strtoull(row[3], NULL, 10);
            counters.c_unsure = strtoull(row[4], NULL, 10);
            counters.c_minefield = strtoull(row[5], NULL, 10);
            counters.c_virus = strtoull(row[6], NULL, 10);
            counters.c_fp = strtoull(row[7], NULL, 10);
            counters.c_fn = strtoull(row[8], NULL, 10);
         }
         mysql_free_result(res);
      }
   }

#endif
#ifdef HAVE_SQLITE3
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;

   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         counters.c_rcvd = sqlite3_column_int64(pStmt, 0);
         counters.c_ham = sqlite3_column_int64(pStmt, 1);
         counters.c_spam = sqlite3_column_int64(pStmt, 2);
         counters.c_possible_spam = sqlite3_column_int64(pStmt, 3);
         counters.c_unsure = sqlite3_column_int64(pStmt, 4);
         counters.c_minefield = sqlite3_column_int64(pStmt, 5);
         counters.c_virus = sqlite3_column_int64(pStmt, 6);
         counters.c_fp = sqlite3_column_int64(pStmt, 7);
         counters.c_fn = sqlite3_column_int64(pStmt, 8);
      }
   }
   sqlite3_finalize(pStmt);
#endif

   return counters;
}


void update_counters(struct session_data *sdata, struct __data *data, struct __counters *counters, struct __config *cfg){
   char buf[MAXBUFSIZE];
#ifdef HAVE_SQLITE3
   char *err=NULL;
#endif

#ifdef HAVE_MEMCACHED
   unsigned long long mc, rcvd;
   struct __counters c;
   char key[MAX_MEMCACHED_KEY_LEN];
   unsigned int flags=0;
   unsigned long now;
   time_t cclock;

   if(cfg->update_counters_to_memcached == 1){

      time(&cclock);
      now = cclock;


      /* increment counters to memcached */

      if(memcached_increment(&(data->memc), MEMCACHED_MSGS_RCVD, strlen(MEMCACHED_MSGS_RCVD), counters->c_rcvd, &mc) == MEMCACHED_SUCCESS){
         rcvd = mc;

         if(counters->c_ham > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_HAM, strlen(MEMCACHED_MSGS_HAM), counters->c_ham, &mc);
         if(counters->c_spam > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_SPAM, strlen(MEMCACHED_MSGS_SPAM), counters->c_spam, &mc);
         if(counters->c_possible_spam > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_POSSIBLE_SPAM, strlen(MEMCACHED_MSGS_POSSIBLE_SPAM), counters->c_possible_spam, &mc);
         if(counters->c_unsure > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_UNSURE, strlen(MEMCACHED_MSGS_UNSURE), counters->c_unsure, &mc);
         if(counters->c_minefield > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_MINEFIELD, strlen(MEMCACHED_MSGS_MINEFIELD), counters->c_minefield, &mc);
         if(counters->c_virus > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_VIRUS, strlen(MEMCACHED_MSGS_VIRUS), counters->c_virus, &mc);
         if(counters->c_fp > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_FP, strlen(MEMCACHED_MSGS_FP), counters->c_fp, &mc);
         if(counters->c_fn > 0) memcached_increment(&(data->memc), MEMCACHED_MSGS_FN, strlen(MEMCACHED_MSGS_FN), counters->c_fn, &mc);


         bzero(&c, sizeof(c)); 

         snprintf(buf, MAXBUFSIZE-1, "%s %s %s %s %s %s %s %s %s %s", MEMCACHED_MSGS_RCVD, MEMCACHED_MSGS_HAM, MEMCACHED_MSGS_SPAM, MEMCACHED_MSGS_POSSIBLE_SPAM, MEMCACHED_MSGS_UNSURE, MEMCACHED_MSGS_MINEFIELD, MEMCACHED_MSGS_VIRUS, MEMCACHED_MSGS_FP, MEMCACHED_MSGS_FN, MEMCACHED_COUNTERS_LAST_UPDATE);

         if(memcached_mget(&(data->memc), buf) == MEMCACHED_SUCCESS){
            while((memcached_fetch_result(&(data->memc), &key[0], &buf[0], &flags))){
               if(!strcmp(key, MEMCACHED_MSGS_RCVD)) c.c_rcvd = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_HAM)) c.c_ham = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_SPAM)) c.c_spam = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_POSSIBLE_SPAM)) c.c_possible_spam = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_UNSURE)) c.c_unsure = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_MINEFIELD)) c.c_minefield = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_VIRUS)) c.c_virus = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_FP)) c.c_fp = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_MSGS_FN)) c.c_fn = strtoull(buf, NULL, 10);
               else if(!strcmp(key, MEMCACHED_COUNTERS_LAST_UPDATE)) mc = strtoull(buf, NULL, 10);
            }


            if(now - mc > cfg->memcached_to_db_interval && c.c_rcvd > 0 && c.c_rcvd >= rcvd){
               snprintf(buf, SMALLBUFSIZE-1, "%ld", now); memcached_set(&(data->memc), MEMCACHED_COUNTERS_LAST_UPDATE, strlen(MEMCACHED_COUNTERS_LAST_UPDATE), buf, strlen(buf), 0, 0);

               snprintf(buf, SMALLBUFSIZE-1, "UPDATE t_counters SET rcvd=%llu, ham=%llu, spam=%llu, possible_spam=%llu, unsure=%llu, minefield=%llu, virus=%llu, fp=%llu, fn=%llu", c.c_rcvd, c.c_ham, c.c_spam, c.c_possible_spam, c.c_unsure, c.c_minefield, c.c_virus, c.c_fp, c.c_fn);

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: update counters: %s", sdata->ttmpfile, buf);

               goto EXEC_SQL;
            }
         }

      }
      else {

         c = load_counters(sdata, cfg);

         snprintf(buf, SMALLBUFSIZE-1, "%ld", now); memcached_add(&(data->memc), MEMCACHED_COUNTERS_LAST_UPDATE, strlen(MEMCACHED_COUNTERS_LAST_UPDATE), buf, strlen(buf), 0, 0);

         snprintf(buf, SMALLBUFSIZE-1, "%llu", c.c_ham + counters->c_ham); memcached_add(&(data->memc), MEMCACHED_MSGS_HAM, strlen(MEMCACHED_MSGS_HAM), buf, strlen(buf), 0, 0);
         snprintf(buf, SMALLBUFSIZE-1, "%llu", c.c_spam + counters->c_spam); memcached_add(&(data->memc), MEMCACHED_MSGS_SPAM, strlen(MEMCACHED_MSGS_SPAM), buf, strlen(buf), 0, 0);
         snprintf(buf, SMALLBUFSIZE-1, "%llu", c.c_possible_spam + counters->c_possible_spam); memcached_add(&(data->memc), MEMCACHED_MSGS_POSSIBLE_SPAM, strlen(MEMCACHED_MSGS_POSSIBLE_SPAM), buf, strlen(buf), 0, 0);
         snprintf(buf, SMALLBUFSIZE-1, "%llu", c.c_unsure + counters->c_unsure); memcached_add(&(data->memc), MEMCACHED_MSGS_UNSURE, strlen(MEMCACHED_MSGS_UNSURE), buf, strlen(buf), 0, 0);
         snprintf(buf, SMALLBUFSIZE-1, "%llu", c.c_minefield + counters->c_minefield); memcached_add(&(data->memc), MEMCACHED_MSGS_MINEFIELD, strlen(MEMCACHED_MSGS_MINEFIELD), buf, strlen(buf), 0, 0);
         snprintf(buf, SMALLBUFSIZE-1, "%llu", c.c_virus + counters->c_virus); memcached_add(&(data->memc), MEMCACHED_MSGS_VIRUS, strlen(MEMCACHED_MSGS_VIRUS), buf, strlen(buf), 0, 0);
         snprintf(buf, SMALLBUFSIZE-1, "%llu", c.c_fp + counters->c_fp); memcached_add(&(data->memc), MEMCACHED_MSGS_FP, strlen(MEMCACHED_MSGS_FP), buf, strlen(buf), 0, 0);
         snprintf(buf, SMALLBUFSIZE-1, "%llu", c.c_fn + counters->c_fn); memcached_add(&(data->memc), MEMCACHED_MSGS_FN, strlen(MEMCACHED_MSGS_FN), buf, strlen(buf), 0, 0);

         snprintf(buf, SMALLBUFSIZE-1, "%llu", c.c_rcvd + counters->c_rcvd); memcached_add(&(data->memc), MEMCACHED_MSGS_RCVD, strlen(MEMCACHED_MSGS_RCVD), buf, strlen(buf), 0, 0);
      }

   }
   else {
#endif
      snprintf(buf, SMALLBUFSIZE-1, "UPDATE t_counters SET rcvd=rcvd+%llu, ham=ham+%llu, spam=spam+%llu, possible_spam=possible_spam+%llu, unsure=unsure+%llu, minefield=minefield+%llu, virus=virus+%llu, fp=fp+%llu, fn=fn+%llu", counters->c_rcvd, counters->c_ham, counters->c_spam, counters->c_possible_spam, counters->c_unsure, counters->c_minefield, counters->c_virus, counters->c_fp, counters->c_fn);

#ifdef HAVE_MEMCACHED
EXEC_SQL:
#endif

   #ifdef HAVE_MYSQL
      mysql_real_query(&(sdata->mysql), buf, strlen(buf));
   #endif
   #ifdef HAVE_SQLITE3
      sqlite3_exec(sdata->db, buf, NULL, NULL, &err);
   #endif

#ifdef HAVE_MEMCACHED
   }
#endif

}


