/*
 * partition.c, SJ
 */

#include <kospam.h>


static int is_existing_partition(MYSQL *conn, char *partition, struct config *cfg){
   int count=0;
   char buf[SMALLBUFSIZE];
   struct query sql;

   if(select_db(conn, "information_schema")){
      syslog(LOG_PRIORITY, "ERROR: %s cannot open db: 'information_schema'", __func__);
      return count;
   }

   snprintf(buf, sizeof(buf)-1, "SELECT COUNT(*) AS num FROM PARTITIONS WHERE TABLE_SCHEMA=? AND TABLE_NAME='%s' AND PARTITION_NAME=?", SQL_HISTORY_TABLE);

   if(prepare_sql_statement(conn, &sql, buf) == ERR) return count;

   p_bind_init(&sql);
   sql.sql[sql.pos] = cfg->mysqldb; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = partition; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

   if(p_exec_stmt(conn, &sql) == ERR) goto ENDE;

   p_bind_init(&sql);
   sql.sql[sql.pos] = (char *)&count; sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;

   p_store_results(&sql);
   p_fetch_results(&sql);
   p_free_results(&sql);

ENDE:
   close_prepared_statement(&sql);

   return count;
}


static int create_partition(struct config *cfg){
   int rc=ERR;
   long offset=0;
   time_t tomorrow;
   char buf[SMALLBUFSIZE], partition[TINYBUFSIZE];
   struct tm *t;

   offset = get_local_timezone_offset();

   time_t now = time(NULL);

   tomorrow = now +    86400 - (now % 86400) - offset    + 86400 - 1;
   t = gmtime(&tomorrow);

   snprintf(partition, sizeof(partition)-1, "p%d%02d%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday);

   snprintf(buf, sizeof(buf)-1, "ALTER TABLE %s ADD PARTITION ( PARTITION %s VALUES LESS THAN (%ld) )", SQL_HISTORY_TABLE, partition, tomorrow);

   MYSQL *conn = open_database(cfg);
   if (conn) {
      if(is_existing_partition(conn, partition, cfg) > 0) return rc;

      if(select_db(conn, cfg->mysqldb)){
         syslog(LOG_PRIORITY, "ERROR: %s select_db(): '%s'", __func__, cfg->mysqldb);
         return rc;
      }

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "partition query: %s", buf);
      p_query(conn, buf);
      close_database(conn);

      rc = OK;
   }

   return rc;
}


static int drop_partition(struct config *cfg){
   int rc=ERR;
   char buf[SMALLBUFSIZE], partition[TINYBUFSIZE];
   struct tm *t;

   time_t now = time(NULL);

   now -= 31*86400;
   t = localtime(&now);

   snprintf(partition, sizeof(partition)-1, "p%d%02d%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday);

   snprintf(buf, sizeof(buf)-1, "ALTER TABLE %s DROP PARTITION %s", SQL_HISTORY_TABLE, partition);

   MYSQL *conn = open_database(cfg);
   if (conn) {

      if(is_existing_partition(conn, partition, cfg) <= 0) return rc;

      if(select_db(conn, cfg->mysqldb)){
         syslog(LOG_PRIORITY, "ERROR: %s select_db(): '%s'", __func__, cfg->mysqldb);
         return rc;
      }

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "partition query: %s", buf);

      p_query(conn, buf);
      close_database(conn);

      rc = OK;
   }

   return rc;
}

void manage_partitions(struct config *cfg) {
   create_partition(cfg);
   drop_partition(cfg);
}
