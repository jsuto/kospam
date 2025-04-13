/*
 * misc.h, SJ
 */

#ifndef _MISC_H
 #define _MISC_H

#include <kospam.h>

long tvdiff(struct timeval a, struct timeval b);
void write_pid_file(char *pidfile);
int drop_privileges(struct passwd *pwd);
void init_session_data(struct session_data *sdata);
int is_dotted_ipv4_address(char *s);

#endif /* _MISC_H */
