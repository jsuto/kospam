/*
 * mysql.c, SJ
 */

#include <kospam.h>


MYSQL *open_database(struct config *cfg){
   int rc=1;

   MYSQL *conn = mysql_init(NULL);
   if (conn == NULL) {
      syslog(LOG_PRIORITY, "ERROR: mysql_init() failed");
      return NULL;
   }

   mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg->mysql_connect_timeout);
   mysql_options(conn, MYSQL_OPT_RECONNECT, (const char*)&rc);

   if(mysql_real_connect(conn, cfg->mysqlhost, cfg->mysqluser, cfg->mysqlpwd, cfg->mysqldb, cfg->mysqlport, cfg->mysqlsocket, 0) == 0){
      syslog(LOG_PRIORITY, "ERROR: cant connect to mysql server: '%s'", cfg->mysqldb);
      return NULL;
   }

   mysql_real_query(conn, "SET NAMES utf8mb4", strlen("SET NAMES utf8mb4"));
   mysql_real_query(conn, "SET CHARACTER SET utf8mb4", strlen("SET CHARACTER SET utf8mb4"));

   return conn;
}


void close_database(MYSQL *conn){
   mysql_close(conn);
}


int select_db(MYSQL *conn, const char *db){
   return mysql_select_db(conn, db);
}


void p_bind_init(struct query *sql){
   int i;

   sql->pos = 0;

   for(i=0; i<MAX_SQL_VARS; i++){
      sql->sql[i] = NULL;
      sql->type[i] = TYPE_UNDEF;
      sql->len[i] = 0;
   }
}


void p_query(MYSQL *conn, char *s){
   if(mysql_real_query(conn, s, strlen(s))){
      syslog(LOG_PRIORITY, "ERROR: mysql_real_query() '%s' (errno: %d)", mysql_error(conn), mysql_errno(conn));
   }
}


int p_exec_stmt(MYSQL *conn, struct query *sql){
   MYSQL_BIND bind[MAX_SQL_VARS];
   unsigned long length[MAX_SQL_VARS];
   int i, ret=ERR;

   unsigned int sql_errno = 0;
   memset(bind, 0, sizeof(bind));

   for(i=0; i<MAX_SQL_VARS; i++){
      if(sql->type[i] > TYPE_UNDEF){


         switch(sql->type[i]) {
             case TYPE_SHORT:
                                  bind[i].buffer_type = MYSQL_TYPE_SHORT;
                                  bind[i].length = 0;
                                  break;

             case TYPE_LONG:
                                  bind[i].buffer_type = MYSQL_TYPE_LONG;
                                  bind[i].length = 0;
                                  break;


             case TYPE_LONGLONG:
                                  bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
                                  bind[i].length = 0;
                                  break;


             case TYPE_STRING:
                                  bind[i].buffer_type = MYSQL_TYPE_STRING;
                                  length[i] = strlen(sql->sql[i]);
                                  bind[i].length = &length[i];
                                  break;


             default:
                                  bind[i].buffer_type = 0;
                                  bind[i].length = 0;
                                  break;

         };


         bind[i].buffer = sql->sql[i];
         bind[i].is_null = 0;

      }
      else { break; }
   }

   if(mysql_stmt_bind_param(sql->stmt, bind)){
      sql_errno = mysql_stmt_errno(sql->stmt);
      syslog(LOG_PRIORITY, "ERROR: mysql_stmt_bind_param() '%s' (errno: %d)", mysql_stmt_error(sql->stmt), sql_errno);
      goto CLOSE;
   }

   if(mysql_stmt_execute(sql->stmt)){
      sql_errno = mysql_stmt_errno(sql->stmt);
      syslog(LOG_PRIORITY, "ERROR: mysql_stmt_execute() '%s' (errno: %d)", mysql_error(conn), sql_errno);
      goto CLOSE;
   }

   ret = OK;

CLOSE:
   return ret;
}


int p_store_results(struct query *sql){
   MYSQL_BIND bind[MAX_SQL_VARS];
   int i, ret=ERR;

   memset(bind, 0, sizeof(bind));

   for(i=0; i<MAX_SQL_VARS; i++){
      if(sql->type[i] > TYPE_UNDEF){

         switch(sql->type[i]) {
             case TYPE_SHORT:     bind[i].buffer_type = MYSQL_TYPE_SHORT;
                                  break;


             case TYPE_LONG:      bind[i].buffer_type = MYSQL_TYPE_LONG;
                                  break;


             case TYPE_LONGLONG:
                                  bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
                                  break;


             case TYPE_STRING:
                                  bind[i].buffer_type = MYSQL_TYPE_STRING;
                                  bind[i].buffer_length = sql->len[i];
                                  break;

             default:
                                  bind[i].buffer_type = 0;
                                  break;

         };


         bind[i].buffer = (char *)sql->sql[i];
         bind[i].is_null = &(sql->is_null[i]);
         bind[i].length = &(sql->length[i]);
         bind[i].error = &(sql->error[i]);

      }
      else { break; }
   }

   if(mysql_stmt_bind_result(sql->stmt, bind)){
      goto CLOSE;
   }


   if(mysql_stmt_store_result(sql->stmt)){
      goto CLOSE;
   }

   ret = OK;

CLOSE:

   return ret;
}


int p_fetch_results(struct query *sql){

   if(mysql_stmt_fetch(sql->stmt) == 0) return OK;

   return ERR;
}


void p_free_results(struct query *sql){
   mysql_stmt_free_result(sql->stmt);
}


uint64 p_get_insert_id(struct query *sql){
   return mysql_stmt_insert_id(sql->stmt);
}


int p_get_affected_rows(struct query *sql){
   return mysql_stmt_affected_rows(sql->stmt);
}


int prepare_sql_statement(MYSQL *conn, struct query *sql, char *s){
   sql->stmt = mysql_stmt_init(conn);
   if(!(sql->stmt)){
      syslog(LOG_PRIORITY, "ERROR: mysql_stmt_init()");
      return ERR;
   }

   if(mysql_stmt_prepare(sql->stmt, s, strlen(s))){
      syslog(LOG_PRIORITY, "ERROR: mysql_stmt_prepare() %s => sql: %s", mysql_stmt_error(sql->stmt), s);
      return ERR;
   }

   return OK;
}


void close_prepared_statement(struct query *sql){
   if(sql->stmt) mysql_stmt_close(sql->stmt);
}


struct te get_ham_spam_counters(MYSQL *conn, char *stmt){
   struct te te;
   MYSQL_RES *res;
   MYSQL_ROW row;

   te.nham = te.nspam = 0;

   if(mysql_real_query(conn, stmt, strlen(stmt)) == 0){
      res = mysql_store_result(conn);
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            te.nham += atof(row[0]);
            te.nspam += atof(row[1]);
         }
         mysql_free_result(res);
      }
   }
   return te;
}
