/*
 * user.h, 2006.07.06, SJ
 */

#include "config.h"

#define SPAMOVERALLLIMIT "spamOverallLimit"
#define EMAIL "Email"
#define CLAPFACTION "clapfAction"
#define CLAPFPAGELEN "clapfPagelen"

struct userpref {
   char email[SMALLBUFSIZE];
   char action[SMALLBUFSIZE];
   char pagelen[SMALLBUFSIZE];
};

struct userpref ldap_get_entry(char *ldap_host, char *ldap_base, char *ldap_user, char *ldap_pwd, int ldap_use_tls, char *user);
int ldap_set_entry(char *ldap_host, char *ldap_base, char *ldap_user, char *ldap_pwd, int ldap_use_tls, char *user, struct userpref u);

struct userpref mysql_get_entry(char *mysqlhost, int mysqlport, char *mysqlsocket, char *mysqluser, char *mysqlpwd, char *mysqldb, char *usertable, char *user);
int mysql_set_entry(char *mysqlhost, int mysqlport, char *mysqlsocket, char *mysqluser, char *mysqlpwd, char *mysqldb, char *usertable, char *user, struct userpref u);
