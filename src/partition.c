/*
 * partition.c, SJ
 */

#include <kospam.h>


static int is_existing_partition(struct session_data *sdata, char *partition, struct __config *cfg){
   int count=0;
   char buf[SMALLBUFSIZE];
   struct sql sql;

   if(select_db(sdata, "information_schema")){
      syslog(LOG_PRIORITY, "ERROR: %s cannot open db: 'information_schema'", __func__);
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

   p_store_results(&sql);
   p_fetch_results(&sql);
   p_free_results(&sql);

ENDE:
   close_prepared_statement(&sql);

   return count;
}


static int create_partition(struct __config *cfg){
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

   snprintf(buf, sizeof(buf)-1, "ALTER TABLE %s ADD PARTITION ( PARTITION %s VALUES LESS THAN (%ld) )", SQL_HISTORY_TABLE, partition, tomorrow);

   if(open_database(&sdata, cfg) == OK){

      if(is_existing_partition(&sdata, partition, cfg) > 0) return rc;

      if(select_db(&sdata, cfg->mysqldb)){
         syslog(LOG_PRIORITY, "ERROR: %s select_db(): '%s'", __func__, cfg->mysqldb);
         return rc;
      }

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "partition query: %s", buf);
      p_query(&sdata, buf);
      close_database(&sdata);

      rc = OK;
   }
   else syslog(LOG_PRIORITY, "ERROR: %s open_database() failed", __func__);

   return rc;
}


static int drop_partition(struct __config *cfg){
   int rc=ERR;
   char buf[SMALLBUFSIZE], partition[TINYBUFSIZE];
   struct tm *t;
   struct session_data sdata;

   init_session_data(&sdata, cfg);

   sdata.now -= 31*86400;
   t = localtime(&(sdata.now));

   snprintf(partition, sizeof(partition)-1, "p%d%02d%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday);

   snprintf(buf, sizeof(buf)-1, "ALTER TABLE %s DROP PARTITION %s", SQL_HISTORY_TABLE, partition);

   if(open_database(&sdata, cfg) == OK){

      if(is_existing_partition(&sdata, partition, cfg) <= 0) return rc;

      if(select_db(&sdata, cfg->mysqldb)){
         syslog(LOG_PRIORITY, "ERROR: %s select_db(): '%s'", __func__, cfg->mysqldb);
         return rc;
      }

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "partition query: %s", buf);

      p_query(&sdata, buf);
      close_database(&sdata);

      rc = OK;
   }
   else syslog(LOG_PRIORITY, "ERROR: %s open_database() failed", __func__);

   return rc;
}

void manage_partitions(struct __config *cfg) {
   create_partition(cfg);
   drop_partition(cfg);
}
