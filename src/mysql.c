/*
 * mysql.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <clapf.h>


int open_database(struct session_data *sdata, struct __config *cfg){
   int rc=1;

   mysql_init(&(sdata->mysql));

   mysql_options(&(sdata->mysql), MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg->mysql_connect_timeout);
   mysql_options(&(sdata->mysql), MYSQL_OPT_RECONNECT, (const char*)&rc);

   if(mysql_real_connect(&(sdata->mysql), cfg->mysqlhost, cfg->mysqluser, cfg->mysqlpwd, cfg->mysqldb, cfg->mysqlport, cfg->mysqlsocket, 0) == 0){
      printf("cant connect to mysql server\n");
      return ERR;
   }

   mysql_real_query(&(sdata->mysql), "SET NAMES utf8", strlen("SET NAMES utf8"));
   mysql_real_query(&(sdata->mysql), "SET CHARACTER SET utf8", strlen("SET CHARACTER SET utf8"));

   return OK;
}


void close_database(struct session_data *sdata){
   mysql_close(&(sdata->mysql));
}


int select_db(struct session_data *sdata, const char *db){
   return mysql_select_db(&(sdata->mysql), db);
}


void p_bind_init(struct sql *sql){
   int i;

   sql->pos = 0;

   for(i=0; i<MAX_SQL_VARS; i++){
      sql->sql[i] = NULL;
      sql->type[i] = TYPE_UNDEF;
      sql->len[i] = 0;
   }
}


void p_query(struct session_data *sdata, char *s){
   if(mysql_real_query(&(sdata->mysql), s, strlen(s))){
      syslog(LOG_PRIORITY, "%s: mysql_real_query() error: %s (errno: %d)", sdata->ttmpfile, mysql_error(&(sdata->mysql)), mysql_errno(&(sdata->mysql)));
   }
}


int p_exec_stmt(struct session_data *sdata, struct sql *sql){
   MYSQL_BIND bind[MAX_SQL_VARS];
   unsigned long length[MAX_SQL_VARS];
   int i, ret=ERR;

   sdata->sql_errno = 0;
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
      sdata->sql_errno = mysql_stmt_errno(sql->stmt);
      syslog(LOG_PRIORITY, "%s: mysql_stmt_bind_param() error: %s (errno: %d)", sdata->ttmpfile, mysql_stmt_error(sql->stmt), sdata->sql_errno);
      goto CLOSE;
   }

   if(mysql_stmt_execute(sql->stmt)){
      sdata->sql_errno = mysql_stmt_errno(sql->stmt);
      syslog(LOG_PRIORITY, "%s: mysql_stmt_execute error: *%s* (errno: %d)", sdata->ttmpfile, mysql_error(&(sdata->mysql)), sdata->sql_errno);
      goto CLOSE;
   }

   ret = OK;

CLOSE:
   return ret;
}


int p_store_results(struct session_data *sdata, struct sql *sql){
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


int p_fetch_results(struct sql *sql){

   if(mysql_stmt_fetch(sql->stmt) == 0) return OK;

   return ERR;
}


void p_free_results(struct sql *sql){
   mysql_stmt_free_result(sql->stmt);
}


uint64 p_get_insert_id(struct sql *sql){
   return mysql_stmt_insert_id(sql->stmt);
}


int p_get_affected_rows(struct sql *sql){
   return mysql_stmt_affected_rows(sql->stmt);
}


int prepare_sql_statement(struct session_data *sdata, struct sql *sql, char *s){

   sql->stmt = mysql_stmt_init(&(sdata->mysql));
   if(!(sql->stmt)){
      syslog(LOG_PRIORITY, "%s: mysql_stmt_init() error", sdata->ttmpfile);
      return ERR;
   }

   if(mysql_stmt_prepare(sql->stmt, s, strlen(s))){
      syslog(LOG_PRIORITY, "%s: mysql_stmt_prepare() error: %s => sql: %s", sdata->ttmpfile, mysql_stmt_error(sql->stmt), s);
      return ERR;
   }

   return OK; 
}


void close_prepared_statement(struct sql *sql){
   if(sql->stmt) mysql_stmt_close(sql->stmt);
}


struct te get_ham_spam_counters(struct session_data *sdata, char *stmt){
   struct te te;
   MYSQL_RES *res;
   MYSQL_ROW row;

   te.nham = te.nspam = 0;

   if(mysql_real_query(&(sdata->mysql), stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&(sdata->mysql));
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


void update_hash(struct session_data *sdata, char *qry, struct node *xhash[]){
   MYSQL_RES *res;
   MYSQL_ROW row;
   float nham, nspam;
   uint64 token;

   if(mysql_real_query(&(sdata->mysql), qry, strlen(qry)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            token = strtoull(row[0], NULL, 10);
            nham = atof(row[1]);
            nspam = atof(row[2]);

            updatenode(xhash, token, nham, nspam, DEFAULT_SPAMICITY, 0);
         }

         mysql_free_result(res);
      }

   }

}


