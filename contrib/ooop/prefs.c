/*
 * prefs.c, 2008.06.04, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sqlite3.h>
#include "misc.h"
#include "messages.h"
#include "pop3.h"
#include "ooop-util.h"
#include "cfg.h"
#include "config.h"
#include <clapf.h>


int get_user_preferences(char *login, char *prefs_db, struct __config *cfg, unsigned long *activation_date, char **whitelist){
   sqlite3 *db;
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   char stmt[MAXBUFSIZE];

   memset(*whitelist, 0, MAXBUFSIZE);

   if(sqlite3_open(prefs_db, &db)) return 0;

   snprintf(stmt, MAXBUFSIZE-1, "SELECT activation_date, max_message_size_to_filter, spam_subject_prefix, rbl_condemns_a_message, surbl_condemns_a_message, whitelist, has_personal_db FROM %s WHERE login='%s'", SQL_POP3_GW_TABLE, login);


   if(sqlite3_prepare_v2(db, stmt, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         *activation_date = sqlite3_column_int(pStmt, 0);

         cfg->max_message_size_to_filter = sqlite3_column_int(pStmt, 1);
         strncpy(cfg->spam_subject_prefix, (char *)sqlite3_column_blob(pStmt, 2), MAXVAL-1);

         cfg->rbl_condemns_the_message = sqlite3_column_int(pStmt, 3);
         cfg->surbl_condemns_the_message = sqlite3_column_int(pStmt, 4);

         strncpy(*whitelist, (char *)sqlite3_column_blob(pStmt, 5), MAXBUFSIZE-1);

         cfg->has_personal_db = sqlite3_column_int(pStmt, 6);
      }
   }
   sqlite3_finalize(pStmt);

   sqlite3_close(db);

   return 1;
}

