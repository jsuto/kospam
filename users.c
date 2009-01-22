/*
 * users.c, 2009.01.22, SJ
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
int get_user_from_email(struct session_data *sdata, char *email, struct __config *cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char *p, buf[MAXBUFSIZE];

   sdata->uid = 0;
   sdata->policy_group = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);

      if((p = strcasestr(email, "+spam"))){
         *p = '\0';
         snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username FROM %s WHERE email='%s%s'", SQL_USER_TABLE, email, p+5);
      }
      else if((p = strcasestr(email, "+ham"))){
         *p = '\0';
         snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username FROM %s WHERE email='%s%s'", SQL_USER_TABLE, email, p+4);
      }
      else
         snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username, policy_group FROM %s WHERE email='%s'", SQL_USER_TABLE, email);


      if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
         res = mysql_store_result(&(sdata->mysql));
         if(res != NULL){
            row = mysql_fetch_row(res);
            if(row){
               sdata->uid = atol(row[0]);
               strncpy(sdata->name, (char *)row[1], SMALLBUFSIZE-1);
               sdata->policy_group = atoi(row[2]);
            }               
            mysql_free_result(res);
         }
      }


   return 1;
}

/*
 * check whether the email address is on the white list
 */

int is_sender_on_white_list(struct session_data *sdata, char *email, struct __config *cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char buf[SMALLBUFSIZE];
   int r=0;

#ifndef HAVE_WHITELIST
   return 0;
#endif

   if(!email) return 0;

   snprintf(buf, SMALLBUFSIZE-1, "SELECT whitelist FROM %s WHERE uid=0 OR uid=%ld", SQL_WHITE_LIST, sdata->uid);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sql: %s", sdata->ttmpfile, buf);

   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL){
         while((row = mysql_fetch_row(res))){
            if(row[0]){
               if(whitelist_check((char *)row[0], sdata->ttmpfile, email, cfg) == 1){
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

#endif


#ifdef USERS_IN_SQLITE3
int get_user_from_email(struct session_data *sdata, char *email, struct __config *cfg){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   struct ue UE;
   char *p, buf[MAXBUFSIZE];

   sdata->uid = 0;
   sdata->policy_group = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);

   if((p = strcasestr(email, "+spam"))){
      *p = '\0';
      snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username FROM %s WHERE email='%s%s'", SQL_USER_TABLE, email, p+5);
   }
   else if((p = strcasestr(email, "+ham"))){
      *p = '\0';
      snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username FROM %s WHERE email='%s%s'", SQL_USER_TABLE, email, p+4);
   }
   else
      snprintf(buf, MAXBUFSIZE-1, "SELECT uid, username FROM %s WHERE email='%s'", SQL_USER_TABLE, email);


   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         //sdata->uid = sqlite3_column_int(pStmt, 0);
         strncpy(sdata->name, (char *)sqlite3_column_blob(pStmt, 1), SMALLBUFSIZE-1);
      }
   }
   sqlite3_finalize(pStmt);

   return 1;
}

int is_sender_on_white_list(struct session_data *sdata, char *email, struct __config *cfg){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   char buf[SMALLBUFSIZE];
   int r=0;

#ifndef HAVE_WHITELIST
   return 0;
#endif

   if(!email) return 0;

   snprintf(buf, SMALLBUFSIZE-1, "SELECT whitelist FROM %s WHERE uid=0 OR uid=%ld", SQL_WHITE_LIST, sdata->uid);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sql: %s", sdata->ttmpfile, buf);

   if(sqlite3_prepare_v2(sdata->db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      while(sqlite3_step(pStmt) == SQLITE_ROW){
         if(whitelist_check((char *)sqlite3_column_blob(pStmt, 0), sdata->ttmpfile, email, cfg) == 1){
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

int get_user_from_email(struct session_data *sdata, char *email, struct __config *cfg){
   int rc;
   char filter[SMALLBUFSIZE], *attrs[] = { NULL }, **vals;
   LDAPMessage *res, *e;

   sdata->uid = 0;
   sdata->policy_group = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);

   if(sdata->ldap == NULL) return 0;

   snprintf(filter, SMALLBUFSIZE-1, "(|(%s=%s)(%s=%s))", cfg->email_address_attribute_name, email, cfg->email_alias_attribute_name, email);

   rc = ldap_search_s(sdata->ldap, cfg->ldap_base, LDAP_SCOPE, filter, attrs, 0, &res);
   if(rc) return 0;

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
   }

   ldap_msgfree(res);

   return 1;
}


int is_sender_on_white_list(struct session_data *sdata, char *email, struct __config *cfg){
   int i, rc, ret=0;
   char filter[SMALLBUFSIZE], *attrs[] = { NULL }, **vals;
   LDAPMessage *res, *e;

   if(sdata->ldap == NULL) return ret;

   snprintf(filter, SMALLBUFSIZE-1, "uid=%ld", sdata->uid);

   rc = ldap_search_s(sdata->ldap, cfg->ldap_base, LDAP_SCOPE, filter, attrs, 0, &res);
   if(rc) return ret;

   e = ldap_first_entry(sdata->ldap, res);

   if(e){
      vals = ldap_get_values(sdata->ldap, e, "filterSender");
      for(i=0; i<ldap_count_values(vals); i++){
         ret = whitelist_check((char *)vals[i], sdata->ttmpfile, email, cfg);
         if(ret == 1) break;
      }
      ldap_value_free(vals);
   }

   ldap_msgfree(res);

   return ret;
}

#endif
