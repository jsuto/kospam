/*
 * policy.h, 2008.11.25, SJ
 */

#ifndef _POLICY_H
 #define _POLICY_H

#include <clapf.h>

#ifdef USERS_IN_MYSQL
   #include <mysql.h>
   int get_policy(MYSQL mysql, struct __config *cfg, struct __config *my_cfg, unsigned int policy_group, int num_of_rcpt_to);
#endif

#endif /* _POLICY_H */
