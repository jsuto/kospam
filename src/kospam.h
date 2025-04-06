/*
 * kospam.h, Janos Suto
 */

#ifndef _KOSPAM_H
 #define _KOSPAM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <locale.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>

#include <sig.h>
#include <sql.h>
#include <misc.h>
#include <defs.h>
#include <hash.h>
#include <chi.h>
#include <buffer.h>
#include <oldparser.h>
#include <errmsg.h>
#include <users.h>
#include <child.h>
#include <config.h>

#define PROGNAME "kospam/filter"

#define DEVIATION(n) fabs((n)-0.5f)

signal_func *set_signal_handler(int signo, signal_func * func);

void disable_coredump();
struct __config read_config(char *configfile);
void check_and_create_directories(struct __config *cfg);
void kill_children(int sig, char *sig_text);

void zombie_init(struct __data *data, struct __config *cfg);
void check_zombie_sender(struct session_data *sdata, struct __data *data, struct __config *cfg);
void zombie_free(struct __data *data);

int check_for_known_bad_attachments(struct session_data *sdata, struct __state *state);
int check_spam(struct session_data *sdata, struct __state *state, struct __data *data, char *fromemail, char *rcpttoemail, struct __config *cfg, struct __config *my_cfg);
void add_penalties(struct session_data *sdata, struct __state *state, struct __config *cfg);

int check_rbl_lists(struct __state *state, char *domainlist);

uint64 xxh3_64(const void *data);

int get_tokens(struct __state *state, char type, struct __config *cfg);
int update_token_dates(struct __state *state, struct __config *cfg);

int train_message(struct session_data *sdata, struct __state *state, char *column, struct __config *cfg);

void update_counters(struct session_data *sdata, struct __counters *counters);


char *split(char *str, int ch, char *buf, int buflen, int *result);
int fix_message_file(const char *filename, struct session_data *sdata, struct __config *cfg);

int get_policy(struct session_data *sdata, struct __config *cfg, struct __config *my_cfg);
float run_statistical_check(struct session_data *sdata, struct __state *state, struct __config *cfg);

void manage_partitions(struct __config *cfg);

int write_history_to_sql(struct session_data *sdata, struct __state *state);

void store_minefield_ip(struct session_data *sdata, char *ip);

#endif /* _KOSPAM_H */
