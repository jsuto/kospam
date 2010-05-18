/*
 * users.h, 2010.05.10, SJ
 */

#ifndef _USERS_H
 #define _USERS_H

#include <clapf.h>
#include "defs.h"


#ifdef USERS_IN_LDAP
   #include <ldap.h>

   #define LDAP_SCOPE LDAP_SCOPE_SUBTREE
   #define LDAP_USE_TLS 1

   LDAP *ldap_init(char *host, int port);
   int ldap_search_s(LDAP *ld, char *base, int scope, char *filter, char **attrs, int attrsonly, LDAPMessage **res);
   void ldap_perror( LDAP *ld, const char *s);
   int ldap_simple_bind_s(LDAP *ld, const char *who, const char *passwd);
   int ldap_count_values(char **vals);
   char **ldap_get_values(LDAP *ld, LDAPMessage *entry, char *attr);
   void ldap_value_free(char **vals);
   int ldap_unbind_s(LDAP *ld);

   LDAP *do_bind_ldap(char *ldap_host, char *binddn, char *bindpw, int usetls);
#endif

int getUserdataFromEmail(struct session_data *sdata, char *email, struct __config *cfg);
int isKnownEmail(struct session_data *sdata, char *email, struct __config *cfg);

#endif /* _USERS_H */

