#ifndef _CHILD_H
 #define _CHILD_H

#include <kospam.h>

extern struct __config cfg;
extern struct __data data;

extern struct child children[MAXCHILDREN];
extern int quit;
extern int received_sighup;

void child_sighup_handler(int sig);
pid_t child_make(struct child *ptr);
int child_pool_create(int number_of_worker_processes);
int search_slot_by_pid(pid_t pid);
void kill_children(int sig, char *sig_text);
void p_clean_exit();
void child_main(struct child *ptr);
void takesig(int sig);
void initialise_configuration();

#endif
