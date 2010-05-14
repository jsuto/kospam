/*
 * users.c, 2010.05.13, SJ
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


#ifdef USERS_IN_MYSQL
   #include <mysql.h>
#endif

#ifdef USERS_IN_SQLITE3
   #include <sqlite3.h>
#endif



/*
 * get uid and username from email address
 */

#ifdef USERS_IN_MYSQL
int getUserdataFromEmail(struct session_data *sdata, char *email, struct __config *cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char *p, *q, buf[MAXBUFSIZE], _email[2*SMALLBUFSIZE+1];
   int rc=0;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: query user data from %s", sdata->ttmpfile, email);

   sdata->uid = 0;
   sdata->policy_group = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);
   memset(sdata->domain, 0, SMALLBUFSIZE);

   if(email == NULL) return 0;


   /* escape the email address, as it comes from the network, 2009.05.19, SJ */
   mysql_real_escape_string(&(sdata->mysql), _email, email, strlen(email));


   /* skip the +... part from the email */

   if((p = strchr(_email, '+'))){
      *p = '\0';
      q = strchr(p+1, '@');

      if(!q) return 0;

      snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s%s'",
         SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, _email, q);

      *p = '+';
   }
   else {
      snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s'",
            SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, _email);
   }


   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: user data stmt: %s", sdata->ttmpfile, buf);


   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL && mysql_num_fields(res) == 4){
         row = mysql_fetch_row(res);
         if(row){
            sdata->uid = atol(row[0]);
            if(row[1]) snprintf(sdata->name, SMALLBUFSIZE-1, "%s", (char *)row[1]);
            if(row[2]) snprintf(sdata->domain, SMALLBUFSIZE-1, "%s", (char *)row[2]);
            sdata->policy_group = atoi(row[3]);
            rc = 1;
         }               
         mysql_free_result(res);
      }
   }

   if(rc == 1) return 0;


   /* if no email was found, then try to lookup the domain */

   p = strchr(_email, '@');
   if(!p) return 0;

   snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s'",
      SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, p);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: user data stmt2: %s", sdata->ttmpfile, buf);

   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL && mysql_num_fields(res) == 4){
         row = mysql_fetch_row(res);
         if(row){
            sdata->uid = atol(row[0]);
            if(row[1]) snprintf(sdata->name, SMALLBUFSIZE-1, "%s", (char *)row[1]);
            if(row[2]) snprintf(sdata->domain, SMALLBUFSIZE-1, "%s", (char *)row[2]);
            sdata->policy_group = atoi(row[3]);
         }
         mysql_free_result(res);
      }
   }

   return 0;
}


/*
 * check whether the email address is on the white list
 */

int isSenderOnBlackOrWhiteList(struct session_data *sdata, char *email, char *fieldname, char *table, struct __config *cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char buf[SMALLBUFSIZE];
   int r=0;

   if(!email) return 0;

   snprintf(buf, SMALLBUFSIZE-1, "SELECT %s FROM %s WHERE uid=0 OR uid=%ld", fieldname, table, sdata->uid);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sql: %s", sdata->ttmpfile, buf);

   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            if(row[0]){
               if(isEmailAddressOnList((char *)row[0], sdata->ttmpfile, email, cfg) == 1){
                  r = 1;
                  break;
               }
            }
         }
         mysql_free_result(res);
      }
   }

   return r;
}


/*
 * is it a valid and known email?
 */

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


#ifdef USERS_IN_SQLITE3
int getUserdataFromEmail(struct session_data *sdata, char *email, struct __config *cfg){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   char *p, *q, buf[MAXBUFSIZE];
   int rc=0;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: query user data from %s", sdata->ttmpfile, email);

   sdata->uid = 0;
   sdata->policy_group = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);
   memset(sdata->domain, 0, SMALLBUFSIZE);

   if(email == NULL) return 0;

   /* skip the +... part from the email */

   if((p = strchr(email, '+'))){
      *p = '\0';
      q = strchr(p+1, '@');

      if(!q) return 0;

      snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s%s'",
         SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, email, q);

      *p = '+';
   }
   else {
      snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.username, %s.domain, %s.policy_group FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s'",
            SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, email);
   }

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: user data stmt: %s", sdata->ttmpfile, buf);

   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         sdata->uid = sqlite3_column_int(pStmt, 0);
         if(sqlite3_column_blob(pStmt, 1)) strncpy(sdata->name, (char *)sqlite3_column_blob(pStmt, 1), SMALLBUFSIZE-1);
         if(sqlite3_column_blob(pStmt, 2)) strncpy(sdata->domain, (char *)sqlite3_column_blob(pStmt, 2), SMALLBUFSIZE-1);
         sdata->policy_group = sqlite3_column_int(pStmt, 3);

         rc = 1;
      }
   }
   sqlite3_finalize(pStmt);

   if(rc == 1) return 0;


   /* if no email was found, then try to lookup the domain */

   p = strchr(email, '@');
   if(!p) return 0;

   snprintf(buf, MAXBUFSIZE-1, "SELECT %s.uid, %s.username, %s.domain FROM %s,%s WHERE %s.uid=%s.uid AND %s.email='%s'",
      SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_USER_TABLE, SQL_EMAIL_TABLE, SQL_EMAIL_TABLE, p);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: user data stmt: %s", sdata->ttmpfile, buf);

   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         sdata->uid = sqlite3_column_int(pStmt, 0);
         if(sqlite3_column_blob(pStmt, 1)) strncpy(sdata->name, (char *)sqlite3_column_blob(pStmt, 1), SMALLBUFSIZE-1);
         if(sqlite3_column_blob(pStmt, 2)) strncpy(sdata->domain, (char *)sqlite3_column_blob(pStmt, 2), SMALLBUFSIZE-1);
         sdata->policy_group = sqlite3_column_int(pStmt, 3);
      }
   }
   sqlite3_finalize(pStmt);


   return 0;
}


int isSenderOnBlackOrWhiteList(struct session_data *sdata, char *email, char *fieldname, char *table, struct __config *cfg){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   char buf[SMALLBUFSIZE];
   int r=0;

   if(!email) return 0;

   snprintf(buf, SMALLBUFSIZE-1, "SELECT %s FROM %s WHERE uid=0 OR uid=%ld", fieldname, table, sdata->uid);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sql: %s", sdata->ttmpfile, buf);

   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      while(sqlite3_step(pStmt) == SQLITE_ROW){
         if(isEmailAddressOnList((char *)sqlite3_column_blob(pStmt, 0), sdata->ttmpfile, email, cfg) == 1){
            r = 1;
            break;
         }
      }
   }
   sqlite3_finalize(pStmt);

   return r;
}

#endif


/* LDAP stuff */

#ifdef USERS_IN_LDAP

/*
 * connect to LDAP server
 */

LDAP *do_bind_ldap(char *ldap_host, char *binddn, char *bindpw, int usetls){
   LDAP *ld;
   const int version = LDAP_VERSION3;

   ld = ldap_init(ldap_host, LDAP_PORT);
      if(ld == NULL){
      ldap_perror(ld, "init");
      return ld;
   }

   if(ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version) != LDAP_OPT_SUCCESS){
      ldap_perror(ld, "setting protocol version");
      return NULL;
   }

   if(usetls == LDAP_USE_TLS){
      if(ldap_start_tls_s(ld, NULL, NULL) != LDAP_SUCCESS){
         ldap_perror(ld, "tls");
         return NULL;
      }
   }

   if(ldap_simple_bind_s(ld, binddn, bindpw) != LDAP_SUCCESS){
      return NULL;
   }

   return ld;
}


/*
 * ask a specific entry about the user from LDAP directory
 */

int getUserdataFromEmail(struct session_data *sdata, char *email, struct __config *cfg){
   int rc=0;
   char filter[SMALLBUFSIZE], *attrs[] = { NULL }, **vals, *p, *q;
   LDAPMessage *res, *e;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: query user data from %s", sdata->ttmpfile, email);

   sdata->uid = 0;
   sdata->policy_group = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);
   memset(sdata->domain, 0, SMALLBUFSIZE);

   if(email == NULL) return 0;

   if(sdata->ldap == NULL) return 0;

   if((p = strchr(email, '+'))){
      *p = '\0';
      q = strchr(p+1, '@');

      if(!q) return 0;

      snprintf(filter, SMALLBUFSIZE-1, "(|(%s=%s%s)(%s=%s%s))", cfg->email_address_attribute_name, email, q, cfg->email_alias_attribute_name, email, q);

      *p = '+';
   }
   else
      snprintf(filter, SMALLBUFSIZE-1, "(|(%s=%s)(%s=%s))", cfg->email_address_attribute_name, email, cfg->email_alias_attribute_name, email);

   rc = ldap_search_s(sdata->ldap, cfg->ldap_base, LDAP_SCOPE, filter, attrs, 0, &res);

   e = ldap_first_entry(sdata->ldap, res);
   if(e){
      vals = ldap_get_values(sdata->ldap, e, "uid");
      if(ldap_count_values(vals) > 0) sdata->uid = atol(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "cn");
      if(ldap_count_values(vals) > 0) strncpy(sdata->name, vals[0], SMALLBUFSIZE-1);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "policyGroupId");
      if(ldap_count_values(vals) > 0) sdata->policy_group = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "domain");
      if(ldap_count_values(vals) > 0) strncpy(sdata->domain, vals[0], SMALLBUFSIZE-1);
      ldap_value_free(vals);

      rc = 1;
   }
   ldap_msgfree(res);

   if(rc == 1) return 0;

   p = strchr(email, '@');
   if(!p) return 0;

   snprintf(filter, SMALLBUFSIZE-1, "(|(%s=%s)(%s=%s))", cfg->email_address_attribute_name, p, cfg->email_alias_attribute_name, p);

   rc = ldap_search_s(sdata->ldap, cfg->ldap_base, LDAP_SCOPE, filter, attrs, 0, &res);

   e = ldap_first_entry(sdata->ldap, res);
   if(e){
      vals = ldap_get_values(sdata->ldap, e, "uid");
      if(ldap_count_values(vals) > 0) sdata->uid = atol(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "cn");
      if(ldap_count_values(vals) > 0) strncpy(sdata->name, vals[0], SMALLBUFSIZE-1);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "policyGroupId");
      if(ldap_count_values(vals) > 0) sdata->policy_group = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "domain");
      if(ldap_count_values(vals) > 0) strncpy(sdata->domain, vals[0], SMALLBUFSIZE-1);
      ldap_value_free(vals);
   }

   ldap_msgfree(res);

   return 0;
}


int isSenderOnBlackOrWhiteList(struct session_data *sdata, char *email, char *fieldname, char *table, struct __config *cfg){
   int i, rc, ret=0;
   char filter[SMALLBUFSIZE], *attrs[] = { NULL }, **vals;
   LDAPMessage *res, *e;

   if(sdata->ldap == NULL) return ret;

   snprintf(filter, SMALLBUFSIZE-1, "uid=%ld", sdata->uid);

   rc = ldap_search_s(sdata->ldap, cfg->ldap_base, LDAP_SCOPE, filter, attrs, 0, &res);
   if(rc) return ret;

   e = ldap_first_entry(sdata->ldap, res);

   if(e){

      if(strcmp(table, SQL_WHITE_LIST) == 0)
         vals = ldap_get_values(sdata->ldap, e, "filterSender");
      else
         vals = ldap_get_values(sdata->ldap, e, "filterMember");

      for(i=0; i<ldap_count_values(vals); i++){
         ret = isEmailAddressOnList((char *)vals[i], sdata->ttmpfile, email, cfg);
         if(ret == 1) break;
      }
      ldap_value_free(vals);
   }

   ldap_msgfree(res);

   return ret;
}

#endif
