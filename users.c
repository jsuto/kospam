/*
 * sql.c, 2008.11.27, SJ
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
struct ue get_user_from_email(MYSQL mysql, char *email){
   MYSQL_RES *res;
   MYSQL_ROW row;
   struct ue UE;
   char *p, buf[MAXBUFSIZE];

   memset((char *)&UE, 0, sizeof(UE));

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


      if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
         res = mysql_store_result(&mysql);
         if(res != NULL){
            row = mysql_fetch_row(res);
            if(row){
               UE.uid = atol(row[0]);
               strncpy(UE.name, (char *)row[1], SMALLBUFSIZE-1);
               UE.policy_group = atoi(row[2]);
            }               
            mysql_free_result(res);
         }
      }


   return UE;

}
#endif


#ifdef USERS_IN_SQLITE3
struct ue get_user_from_email(sqlite3 *db, char *email){
   sqlite3_stmt *pStmt;
   const char **pzTail=NULL;
   struct ue UE;
   char *p, buf[MAXBUFSIZE];

   memset((char *)&UE, 0, sizeof(UE));

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


   if(sqlite3_prepare_v2(db, buf, -1, &pStmt, pzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW){
         UE.uid = sqlite3_column_int(pStmt, 0);
         strncpy(UE.name, (char *)sqlite3_column_blob(pStmt, 1), SMALLBUFSIZE-1);
      }
   }
   sqlite3_finalize(pStmt);


   return UE;
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

struct ue get_user_from_email(LDAP *ld, char *base, char *email, struct __config cfg){
   int rc;
   char filter[SMALLBUFSIZE], *attrs[] = { NULL }, **vals;
   LDAPMessage *res, *e;
   struct ue UE;

   memset((char *)&UE, 0, sizeof(UE));

   if(ld == NULL) return UE;

   snprintf(filter, SMALLBUFSIZE-1, "(|(%s=%s)(%s=%s))", cfg.email_address_attribute_name, email, cfg.email_alias_attribute_name, email);

   rc = ldap_search_s(ld, base, LDAP_SCOPE, filter, attrs, 0, &res);
   if(rc) return UE;

   e = ldap_first_entry(ld, res);

   if(e){
      vals = ldap_get_values(ld, e, "uid");
      if(ldap_count_values(vals) > 0) UE.uid = atol(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "cn");
      if(ldap_count_values(vals) > 0) strncpy(UE.name, vals[0], SMALLBUFSIZE-1);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "policyGroupId");
      if(ldap_count_values(vals) > 0) UE.policy_group = atoi(vals[0]);
      ldap_value_free(vals);
   }

   ldap_msgfree(res);

   return UE;
}

#endif
