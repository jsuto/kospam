/*
 * user.c, 2007.12.28, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "misc.h"
#include "cfg.h"
#include "user.h"
#include "parser.h"
#include "config.h"


int inject_mail(struct session_data sdata, int msg, char *smtpaddr, int smtpport, char *spaminessbuf, struct __config cfg, char *notify);

int deliver_message(char *dir, char *message, char *username, struct __config cfg){
   char sbuf[SMALLBUFSIZE];
   int i;
   struct userpref u;
   struct session_data sdata;

   for(i=0; i<MAX_RCPT_TO; i++){
      memset(sdata.rcptto[i], 0, MAXBUFSIZE);
   }

   sdata.num_of_rcpt_to = 1;

   if(!getenv("REMOTE_USER"))
      return ERR_INJECT;

#ifdef HAVE_USER_LDAP
   u = ldap_get_entry(cfg.ldap_host, cfg.ldap_base, cfg.ldap_user, cfg.ldap_pwd, cfg.ldap_use_tls, username);
#endif

#ifdef HAVE_USER_MYSQL
   u = mysql_get_entry(cfg.mysqlhost, cfg.mysqlport, cfg.mysqlsocket, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, SQL_USER_TABLE, username);
#endif

   /* we got no email address back, 2006.04.11, SJ */
   if(strlen(u.email) < 2)
      return ERR_INJECT;

   snprintf(sdata.rcptto[0], MAXBUFSIZE-1, "RCPT TO: <%s>\r\n", u.email);

   snprintf(sdata.mailfrom, MAXBUFSIZE-1, "MAIL FROM: <%s>\r\n", cfg.spam_mail_from);

   snprintf(sbuf, SMALLBUFSIZE-1, "%sdelivered from spam quarantine\r\n", cfg.clapf_header_field);

   snprintf(sdata.ttmpfile, 3*RND_STR_LEN-1, "%s", message);

   i = inject_mail(sdata, 0, cfg.spam_smtp_addr, cfg.spam_smtp_port, sbuf, cfg, NULL);

   return i;
}

#ifdef HAVE_USER_LDAP

 #include <ldap.h>

 #define LDAP_SCOPE LDAP_SCOPE_BASE
 #define LDAP_USE_TLS 1

 const int version = LDAP_VERSION3;

 LDAP *ldap_init(char *host, int port);
 int ldap_search_s(LDAP *ld, char *base, int scope, char *filter, char **attrs, int attrsonly, LDAPMessage **res);
 void ldap_perror( LDAP *ld, const char *s);
 int ldap_simple_bind_s(LDAP *ld, const char *who, const char *passwd);
 char **ldap_get_values(LDAP *ld, LDAPMessage *entry, char *attr);
 int ldap_count_values(char **vals);
 void ldap_value_free(char **vals);
 int ldap_modify_s(LDAP *ld, char *dn, LDAPMod **mods);
 int ldap_unbind_s(LDAP *ld);


/*
 * connect to LDAP server
 */

 LDAP *do_bind_ldap(char *ldap_host, char *binddn, char *bindpw, int usetls){
   LDAP *ld;

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
 * ask a specific entry of the user from LDAP directory
 */


struct userpref ldap_get_entry(char *ldap_host, char *ldap_base, char *ldap_user, char *ldap_pwd, int ldap_use_tls, char *user){
   LDAPMessage *res, *e;
   LDAP *ld;
   char *attrs[] = { NULL }, **vals, dn[SMALLBUFSIZE];
   int rc;
   struct userpref u;

   memset((char *)&u, 0, sizeof(u));

   ld = do_bind_ldap(ldap_host, ldap_user, ldap_pwd, ldap_use_tls);
   if(ld){
      snprintf(dn, SMALLBUFSIZE-1, "cn=%s,%s", user, ldap_base);

      rc = ldap_search_s(ld, dn, LDAP_SCOPE, NULL, attrs, 0, &res);
      if(!rc){
         e = ldap_first_entry(ld, res);

         vals = ldap_get_values(ld, e, EMAIL);
         if(ldap_count_values(vals) > 0){
            strncpy(u.email, vals[0], SMALLBUFSIZE-1);
         }
         ldap_value_free(vals);

         vals = ldap_get_values(ld, e, CLAPFACTION);
         if(ldap_count_values(vals) > 0){
            strncpy(u.action, vals[0], SMALLBUFSIZE-1);
         }
         ldap_value_free(vals);

         vals = ldap_get_values(ld, e, CLAPFPAGELEN);
         if(ldap_count_values(vals) > 0){
            strncpy(u.pagelen, vals[0], SMALLBUFSIZE-1);
         }
         ldap_value_free(vals);

         ldap_msgfree(res);
      }

      ldap_unbind_s(ld);
   }

   return u;
}

int ldap_set_entry(char *ldap_host, char *ldap_base, char *ldap_user, char *ldap_pwd, int ldap_use_tls, char *user, struct userpref u){
   LDAPMod *mods[4], email, action, pagelen;
   LDAP *ld;
   char dn[SMALLBUFSIZE], *emaill[] = { u.email, NULL }, *actionn[] = { u.action, NULL }, *pagelenn[] = { u.pagelen, NULL };
   int rc, r = ERR;

   ld = do_bind_ldap(ldap_host, ldap_user, ldap_pwd, ldap_use_tls);
   if(ld){
      snprintf(dn, SMALLBUFSIZE-1, "cn=%s,%s", user, ldap_base);


      email.mod_type = EMAIL;
      email.mod_op = LDAP_MOD_REPLACE;
      email.mod_values = emaill;

      action.mod_type = CLAPFACTION;
      action.mod_op = LDAP_MOD_REPLACE;
      action.mod_values = actionn;

      pagelen.mod_type = CLAPFPAGELEN;
      pagelen.mod_op = LDAP_MOD_REPLACE;
      pagelen.mod_values = pagelenn;

      mods[0] = &email;
      mods[1] = &action;
      mods[2] = &pagelen;
      mods[3] = NULL;

      rc = ldap_modify_s(ld, dn, mods);
      if(rc == LDAP_SUCCESS)
         r = OK;

      ldap_unbind_s(ld);
   }

   return r;
}

#endif /* HAVE_USER_LDAP */



#ifdef HAVE_USER_MYSQL

#include <mysql.h>

struct userpref mysql_get_entry(char *mysqlhost, int mysqlport, char *mysqlsocket, char *mysqluser, char *mysqlpwd, char *mysqldb, char *usertable, char *user){
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;
   char buf[MAXBUFSIZE];
   struct userpref u;

   memset((char *)&u, 0, sizeof(u));

   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, mysqlhost, mysqluser, mysqlpwd, mysqldb, mysqlport, mysqlsocket, 0))
      return u;

   snprintf(buf, MAXBUFSIZE-1, "select email, action, pagelen from %s where username='%s'", usertable, user);

   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            strncpy(u.email, row[0], SMALLBUFSIZE-1);
            strncpy(u.action, row[1], SMALLBUFSIZE-1);
            strncpy(u.pagelen, row[2], SMALLBUFSIZE-1);
         }

         mysql_free_result(res);
      }
   }

   mysql_close(&mysql);

   return u;
}

int mysql_set_entry(char *mysqlhost, int mysqlport, char *mysqlsocket, char *mysqluser, char *mysqlpwd, char *mysqldb, char *usertable, char *user, struct userpref u){
   MYSQL mysql;
   char buf[MAXBUFSIZE];
   int r = ERR;

   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, mysqlhost, mysqluser, mysqlpwd, mysqldb, mysqlport, mysqlsocket, 0))
      return ERR;

   snprintf(buf, MAXBUFSIZE-1, "update %s set email='%s', action='%s', pagelen=%d where username='%s'", usertable, u.email, u.action, atoi(u.pagelen), user);

   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      r = OK;
   }

   mysql_close(&mysql);

   return r;
}

#endif /* HAVE_USER_MYSQL */
