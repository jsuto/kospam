/*
 * policy.h, 2009.01.20, SJ
 */

#ifndef _POLICY_H
 #define _POLICY_H

#include <clapf.h>

#ifdef USERS_IN_MYSQL
   #include <mysql.h>
   int get_policy(struct session_data *sdata, struct __config *cfg, struct __config *my_cfg, unsigned int policy_group);
#endif

#ifdef USERS_IN_LDAP
   #include <ldap.h>
   int get_policy(LDAP *ld, char *base, struct __config *cfg, struct __config *my_cfg, unsigned int policy_group, int num_of_rcpt_to);
#endif

#endif /* _POLICY_H */
