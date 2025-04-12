/*
 * misc.h, SJ
 */

#ifndef _MISC_H
 #define _MISC_H

#include <kospam.h>

long tvdiff(struct timeval a, struct timeval b);
int count_character_in_buffer(char *p, char c);
void replace_character_in_buffer(char *p, char from, char to);
char *split_str(char *row, char *what, char *s, int size);
void write_pid_file(char *pidfile);
int drop_privileges(struct passwd *pwd);
void init_session_data(struct session_data *sdata);
int is_dotted_ipv4_address(char *s);

#endif /* _MISC_H */
