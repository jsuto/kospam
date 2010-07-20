/*
 * rbl.h, SJ
 */

#ifndef _RBL_H
 #define _RBL_H

int isIPv4AddressOnRBL(char *ipaddr, char *domainlist);
int isURLOnRBL(char *url, char *domainlist);
int reverseIPv4Address(char *ipaddr);
int checkHostOnRBL(char *host, char *rbldomain);
void checkLists(struct session_data *sdata, struct _state *state, int *found_on_rbl, int *surbl_match, struct __config *cfg);

#endif /* _RBL_H */
