/*
 * users.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <clapf.h>


#ifdef HAVE_MYSQL
int getUserdataFromEmail(struct session_data *sdata, char *email, struct __config *cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char *p, *q, buf[MAXBUFSIZE], _email[2*SMALLBUFSIZE+1];
   int rc=0;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: query user data from %s", sdata->ttmpfile, email);

   sdata->uid = 0;
   sdata->gid = 0;
   sdata->policy_group = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);
   memset(sdata->domain, 0, SMALLBUFSIZE);

   if(email == NULL) return 0;


   mysql_real_escape_string(&(sdata->mysql), _email, email, strlen(email));


   /* skip the +... part from the email */

   if((p = strchr(_email, '+'))){
      *p = '\0';
      q = strchr(p+1, '@');

      if(!q) return 0;

      snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.gid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s%s'",
         SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, _email, q);

      *p = '+';
   }
   else {
      snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.gid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s'",
            SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, _email);
   }


   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: user data stmt: %s", sdata->ttmpfile, buf);


   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL && mysql_num_fields(res) == 5){
         row = mysql_fetch_row(res);
         if(row){
            sdata->uid = atol(row[0]);
            sdata->gid = atoi(row[1]);
            if(row[2]) snprintf(sdata->name, SMALLBUFSIZE-1, "%s", (char *)row[2]);
            if(row[3]) snprintf(sdata->domain, SMALLBUFSIZE-1, "%s", (char *)row[3]);
            sdata->policy_group = atoi(row[4]);
            rc = 1;
         }               
         mysql_free_result(res);
      }
   }

   if(rc == 1) return 0;


   /* if no email was found, then try to lookup the domain */

   p = strchr(_email, '@');
   if(!p) return 0;

   snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.gid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s'",
      SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, p);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: user data stmt2: %s", sdata->ttmpfile, buf);

   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL && mysql_num_fields(res) == 5){
         row = mysql_fetch_row(res);
         if(row){
            sdata->uid = atol(row[0]);
            sdata->gid = atoi(row[1]);
            if(row[2]) snprintf(sdata->name, SMALLBUFSIZE-1, "%s", (char *)row[2]);
            if(row[3]) snprintf(sdata->domain, SMALLBUFSIZE-1, "%s", (char *)row[3]);
            sdata->policy_group = atoi(row[4]);
         }
         mysql_free_result(res);
      }
   }

   return 0;
}


void getWBLData(struct session_data *sdata, struct __config *cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char buf[SMALLBUFSIZE];

   memset(sdata->whitelist, 0, MAXBUFSIZE);
   memset(sdata->blacklist, 0, MAXBUFSIZE);

   snprintf(buf, SMALLBUFSIZE-1, "SELECT whitelist, blacklist FROM %s,%s where (%s.uid=%ld or %s.uid=0) and %s.uid=%s.uid", SQL_WHITE_LIST, SQL_BLACK_LIST, SQL_WHITE_LIST, sdata->uid, SQL_WHITE_LIST, SQL_WHITE_LIST, SQL_BLACK_LIST);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sql: %s", sdata->ttmpfile, buf);

   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL){
         while((row = mysql_fetch_row(res))){

            if(row[0]){
               if(strlen(sdata->whitelist) > 0) strncat(sdata->whitelist, "\n", MAXBUFSIZE-1);
               strncat(sdata->whitelist, (char *)row[0], MAXBUFSIZE-1);
            }

            if(row[1]){
               if(strlen(sdata->blacklist) > 0) strncat(sdata->blacklist, "\n", MAXBUFSIZE-1);
               strncat(sdata->blacklist, (char *)row[1], MAXBUFSIZE-1);
            }
         }
         mysql_free_result(res);
      }
   }

}


int isKnownEmail(struct session_data *sdata, char *email, struct __config *cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char buf[SMALLBUFSIZE], _email[2*SMALLBUFSIZE+1];
   int rc=0;

   if(email == NULL) return rc;

   mysql_real_escape_string(&(sdata->mysql), _email, email, strlen(email));

   snprintf(buf, SMALLBUFSIZE-1, "SELECT COUNT(*) FROM %s WHERE email='%s'", SQL_EMAIL_TABLE, _email);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: is valid email sql: %s", sdata->ttmpfile, buf);

   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row && atoi(row[0]) == 1){
            rc = 1;
         }
         mysql_free_result(res);
      }
   }

   return rc;
}

#endif


#ifdef HAVE_SQLITE3
int getUserdataFromEmail(struct session_data *sdata, char *email, struct __config *cfg){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   char *p, *q, buf[MAXBUFSIZE];
   int rc=0;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: query user data from %s", sdata->ttmpfile, email);

   sdata->uid = 0;
   sdata->gid = 0;
   sdata->policy_group = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);
   memset(sdata->domain, 0, SMALLBUFSIZE);

   if(email == NULL) return 0;

   /* skip the +... part from the email */

   if((p = strchr(email, '+'))){
      *p = '\0';
      q = strchr(p+1, '@');

      if(!q) return 0;

      snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.gid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s%s'",
         SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, email, q);

      *p = '+';
   }
   else {
      snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.gid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s'",
            SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, email);
   }

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: user data stmt: %s", sdata->ttmpfile, buf);

   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         sdata->uid = sqlite3_column_int(pStmt, 0);
         sdata->gid = sqlite3_column_int(pStmt, 1);
         if(sqlite3_column_blob(pStmt, 2)) strncpy(sdata->name, (char *)sqlite3_column_blob(pStmt, 2), SMALLBUFSIZE-1);
         if(sqlite3_column_blob(pStmt, 3)) strncpy(sdata->domain, (char *)sqlite3_column_blob(pStmt, 3), SMALLBUFSIZE-1);
         sdata->policy_group = sqlite3_column_int(pStmt, 4);

         rc = 1;
      }
   }
   sqlite3_finalize(pStmt);

   if(rc == 1) return 0;


   /* if no email was found, then try to lookup the domain */

   p = strchr(email, '@');
   if(!p) return 0;

   snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.gid, %s.username, %s.domain FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s'",
      SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, p);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: user data stmt: %s", sdata->ttmpfile, buf);

   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         sdata->uid = sqlite3_column_int(pStmt, 0);
         sdata->gid = sqlite3_column_int(pStmt, 1);
         if(sqlite3_column_blob(pStmt, 2)) strncpy(sdata->name, (char *)sqlite3_column_blob(pStmt, 2), SMALLBUFSIZE-1);
         if(sqlite3_column_blob(pStmt, 3)) strncpy(sdata->domain, (char *)sqlite3_column_blob(pStmt, 3), SMALLBUFSIZE-1);
         sdata->policy_group = sqlite3_column_int(pStmt, 4);
      }
   }
   sqlite3_finalize(pStmt);


   return 0;
}


void getWBLData(struct session_data *sdata, struct __config *cfg){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   char buf[SMALLBUFSIZE];

   memset(sdata->whitelist, 0, MAXBUFSIZE);
   memset(sdata->blacklist, 0, MAXBUFSIZE);

   snprintf(buf, SMALLBUFSIZE-1, "SELECT whitelist, blacklist FROM %s,%s where (%s.uid=%ld or %s.uid=0) and %s.uid=%s.uid", SQL_WHITE_LIST, SQL_BLACK_LIST, SQL_WHITE_LIST, sdata->uid, SQL_WHITE_LIST, SQL_WHITE_LIST, SQL_BLACK_LIST);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sql: %s", sdata->ttmpfile, buf);

   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) != SQLITE_OK) return;

   while(sqlite3_step(pStmt) == SQLITE_ROW){

      if(strlen(sdata->whitelist) > 0) strncat(sdata->whitelist, "\n", MAXBUFSIZE-1);
      if(sqlite3_column_blob(pStmt, 0)) strncat(sdata->whitelist, (char *)sqlite3_column_blob(pStmt, 0), MAXBUFSIZE-1);

      if(strlen(sdata->blacklist) > 0) strncat(sdata->blacklist, "\n", MAXBUFSIZE-1);
      if(sqlite3_column_blob(pStmt, 1)) strncat(sdata->blacklist, (char *)sqlite3_column_blob(pStmt, 1), MAXBUFSIZE-1);
   }

   sqlite3_finalize(pStmt);
}

#endif


