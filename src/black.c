/*
 * black.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h>
#include "misc.h"
#include "config.h"


#ifdef HAVE_MYSQL

int store_minefield_ip(struct session_data *sdata, struct __config *cfg){
   char stmt[SMALLBUFSIZE], _ip[2*SMALLBUFSIZE+1];
   time_t cclock;
   unsigned long now;

   time(&cclock);
   now = cclock;

   mysql_real_escape_string(&(sdata->mysql), _ip, sdata->client_addr, strlen(sdata->client_addr));

   snprintf(stmt, SMALLBUFSIZE-1, "REPLACE INTO %s (ip, ts) VALUES('%s', %ld)", SQL_MINEFIELD_TABLE, _ip, now);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: minefield store: %s", sdata->ttmpfile, stmt);

   if(mysql_real_query(&(sdata->mysql), stmt, strlen(stmt)) == 0)
      return 1;

   return 0;
}


void is_sender_on_minefield(struct session_data *sdata, char *ip, struct __config *cfg){
   char stmt[SMALLBUFSIZE], _ip[2*SMALLBUFSIZE+1];
   MYSQL_RES *res;
   MYSQL_ROW row;

   mysql_real_escape_string(&(sdata->mysql), _ip, ip, strlen(ip));
   
   snprintf(stmt, SMALLBUFSIZE-1, "SELECT ts FROM %s WHERE ip='%s'", SQL_MINEFIELD_TABLE, _ip);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: minefield query: %s", sdata->ttmpfile, stmt);

   if(mysql_real_query(&(sdata->mysql), stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            sdata->trapped_client = 1;
            syslog(LOG_PRIORITY, "%s: %s is trapped on minefield", sdata->ttmpfile, ip);
         }
         mysql_free_result(res);
      }
   }
}

#endif


#ifdef HAVE_SQLITE3

int store_minefield_ip(struct session_data *sdata, struct __config *cfg){
   char stmt[SMALLBUFSIZE], *err=NULL;
   time_t cclock;
   unsigned long now;

   time(&cclock);
   now = cclock;

   snprintf(stmt, SMALLBUFSIZE-1, "REPLACE INTO %s (ip, ts) VALUES('%s', %ld)", SQL_MINEFIELD_TABLE, sdata->client_addr, now);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: minefield store: %s", sdata->ttmpfile, stmt);

   if((sqlite3_exec(sdata->db, stmt, NULL, NULL, &err)) != SQLITE_OK)
      return 0;

   return 1;
}


void is_sender_on_minefield(struct session_data *sdata, char *ip, struct __config *cfg){
   char stmt[SMALLBUFSIZE];
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;

   snprintf(stmt, MAXBUFSIZE-1, "SELECT ts FROM %s WHERE ip='%s'", SQL_MINEFIELD_TABLE, ip);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: minefield query: %s", sdata->ttmpfile, stmt);

   if(sqlite3_prepare_v2(sdata->db, stmt, -1, &pStmt, pzTail) != SQLITE_OK){
      syslog(LOG_PRIORITY, "%s: error compiling query: %s", sdata->ttmpfile, stmt);
      return;
   }
 
   if(sqlite3_step(pStmt) == SQLITE_ROW){
      sdata->trapped_client = 1;
      syslog(LOG_PRIORITY, "%s: %s is trapped on minefield", sdata->ttmpfile, ip);
   }

   sqlite3_finalize(pStmt);
}

#endif
