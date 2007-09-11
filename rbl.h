/*
 * rbl.h, 2007.09.11, SJ
 */

int rbl_check(char *rbldomain, char *host);
int reverse_ipv4_addr(char *ip);
int rbl_list_check(char *domainlist, char *host);

