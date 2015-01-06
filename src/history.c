/*
 * history.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <clapf.h>


int store_file_to_quarantine(char *filename, struct __config *cfg){
   int rc=ERR;
   char *p0, *p1, *p2, s[SMALLBUFSIZE];
   struct stat st;

   /* create a filename in the store based on piler_id */

   snprintf(s, sizeof(s)-1, "%s/%02x/%c%c/%c%c/%s", cfg->queuedir, cfg->server_id, filename[RND_STR_LEN-4], filename[RND_STR_LEN-3], filename[RND_STR_LEN-2], filename[RND_STR_LEN-1], filename);

   p0 = strrchr(s, '/'); if(!p0) return rc;
   *p0 = '\0';

   if(stat(s, &st)){
      p1 = strrchr(s, '/'); if(!p1) return rc;
      *p1 = '\0';
      p2 = strrchr(s, '/'); if(!p2) return rc;
      *p2 = '\0';

      rc = mkdir(s, 0755);
      *p2 = '/';
      rc = mkdir(s, 0755);
      *p1 = '/';
      rc = mkdir(s, 0755); if(rc == -1) syslog(LOG_PRIORITY, "%s: mkdir %s: error=%s", filename, s, strerror(errno));
   }

   *p0 = '/';

   rc = ERR;

   if(link(filename, s) == 0) rc = OK;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: saving to queue: %s", filename, s);

   return rc;
}


int write_history(struct session_data *sdata, struct __state *state, char *status, struct __config *cfg){
   int i, rc = 0; 
   char relay[SMALLBUFSIZE];
   struct sql sql;

   if(cfg->store_emails == 1 && (cfg->store_only_spam == 0 || sdata->spaminess < cfg->spam_overall_limit) ) store_file_to_quarantine(sdata->ttmpfile, cfg);

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_INSERT_INTO_HISTORY) == ERR) return rc;

   snprintf(relay, sizeof(relay)-1, "%s:%d", cfg->smtp_addr, cfg->smtp_port);

   for(i=0; i<sdata->num_of_rcpt_to; i++){
      p_bind_init(&sql);

      sql.sql[sql.pos] = (char *)&(sdata->now); sql.type[sql.pos] = TYPE_LONG; sql.pos++;
      sql.sql[sql.pos] = sdata->ttmpfile; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
      sql.sql[sql.pos] = (char *)&(sdata->status); sql.type[sql.pos] = TYPE_LONG; sql.pos++;
      sql.sql[sql.pos] = sdata->fromemail; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
      sql.sql[sql.pos] = sdata->rcptto[i]; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
      sql.sql[sql.pos] = state->b_subject; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
      sql.sql[sql.pos] = (char *)&(sdata->tot_len); sql.type[sql.pos] = TYPE_LONG; sql.pos++;
      sql.sql[sql.pos] = (char *)&(state->n_attachments); sql.type[sql.pos] = TYPE_LONG; sql.pos++;
      sql.sql[sql.pos] = &relay[0]; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
      sql.sql[sql.pos] = status; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

      if(p_exec_stmt(sdata, &sql) == OK) rc = 1;
   }


   close_prepared_statement(&sql);

   return rc;
}


int is_existing_partition(struct session_data *sdata, char *partition, struct __config *cfg){
   int count=0;
   char buf[SMALLBUFSIZE];
   struct sql sql;

   if(select_db(sdata, "information_schema")){
      syslog(LOG_PRIORITY, "error: cannot open db: 'information_schema'");
      return count;
   }

   snprintf(buf, sizeof(buf)-1, "SELECT COUNT(*) AS num FROM PARTITIONS WHERE TABLE_SCHEMA=? AND TABLE_NAME='%s' AND PARTITION_NAME=?", SQL_HISTORY_TABLE);

   if(prepare_sql_statement(sdata, &sql, buf) == ERR) return count;

   p_bind_init(&sql);
   sql.sql[sql.pos] = cfg->mysqldb; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = partition; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

   if(p_exec_stmt(sdata, &sql) == ERR) goto ENDE;

   p_bind_init(&sql);
   sql.sql[sql.pos] = (char *)&count; sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;

   p_store_results(sdata, &sql);
   p_fetch_results(&sql);
   p_free_results(&sql);

ENDE:
   close_prepared_statement(&sql);

   return count;
}


int create_partition(struct __config *cfg){
   int rc=ERR;
   long offset=0;
   time_t tomorrow;
   char buf[SMALLBUFSIZE], partition[TINYBUFSIZE];
   struct tm *t;
   struct session_data sdata;

   offset = get_local_timezone_offset();

   init_session_data(&sdata, cfg);

   tomorrow = sdata.now +    86400 - (sdata.now % 86400) - offset    + 86400 - 1;
   t = gmtime(&tomorrow);

   snprintf(partition, sizeof(partition)-1, "p%d%02d%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday);

   snprintf(buf, sizeof(buf)-1, "ALTER TABLE `%s` ADD PARTITION ( PARTITION %s VALUES LESS THAN (%ld) )", SQL_HISTORY_TABLE, partition, tomorrow);

   if(open_database(&sdata, cfg) == OK){

      if(is_existing_partition(&sdata, partition, cfg) > 0) return rc;

      if(select_db(&sdata, cfg->mysqldb)){
         syslog(LOG_PRIORITY, "error: cannot open db: '%s'", cfg->mysqldb);
         return rc;
      }

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "partition query: %s", buf);
      p_query(&sdata, buf);
      close_database(&sdata);

      rc = OK;
   }
   else syslog(LOG_PRIORITY, "error: open db failed in create_partition()");

   return rc;
}


int drop_partition(struct __config *cfg){
   int rc=ERR;
   char buf[SMALLBUFSIZE], partition[TINYBUFSIZE];
   struct tm *t;
   struct session_data sdata;

   init_session_data(&sdata, cfg);

   sdata.now -= 31*86400;
   t = localtime(&(sdata.now));

   snprintf(partition, sizeof(partition)-1, "p%d%02d%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday);

   snprintf(buf, sizeof(buf)-1, "ALTER TABLE `%s` DROP PARTITION %s", SQL_HISTORY_TABLE, partition);

   if(open_database(&sdata, cfg) == OK){

      if(is_existing_partition(&sdata, partition, cfg) <= 0) return rc;

      if(select_db(&sdata, cfg->mysqldb)){
         syslog(LOG_PRIORITY, "error: cannot open db: '%s'", cfg->mysqldb);
         return rc;
      }

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "partition query: %s", buf);
      p_query(&sdata, buf);
      close_database(&sdata);

      rc = OK;
   }
   else syslog(LOG_PRIORITY, "error: open db failed in drop_partition()");

   return rc;
}

