/*
 * kospam.h, Janos Suto
 */

#ifndef _KOSPAM_H
 #define _KOSPAM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <locale.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <getopt.h>
#include <math.h>
#include <zlib.h>
#include <pwd.h>
#include <sys/resource.h>
#include <mysql.h>
#include <mysqld_error.h>

#include <sig.h>
#include <sql.h>
#include <misc.h>
#include <defs.h>
#include <hash.h>
#include <chi.h>
#include <parser.h>
#include <errmsg.h>
#include <users.h>
#include <child.h>
#include <cfg.h>
#include <config.h>

#define PROGNAME "kospam/filter"

#define DEVIATION(n) fabs((n)-0.5f)

typedef void signal_func (int);

signal_func *set_signal_handler(int signo, signal_func * func);

void disable_coredump();
struct config read_config(char *configfile);
void check_and_create_directories(struct config *cfg);
void kill_children(int sig, char *sig_text);

void zombie_init(struct data *data, struct config *cfg);
void check_zombie_sender(struct parser_state *state, struct data *data, struct config *cfg);
void zombie_free(struct data *data);

int check_for_known_bad_attachments(struct session_data *sdata, struct parser_state *state);
int check_spam(struct session_data *sdata, MYSQL *conn, struct parser_state *state, struct data *data, struct config *cfg);
void add_penalties(struct session_data *sdata, struct parser_state *state, struct config *cfg);

int check_rbl_lists(struct parser_state *state, char *domainlist);

uint64 xxh3_64(const void *data);

int get_tokens(struct parser_state *state, char type, struct config *cfg);
int update_token_dates(struct parser_state *state, struct config *cfg);

int train_message(struct parser_state *state, char *column, struct config *cfg);

void update_counters(MYSQL *conn, struct counters *counters);


char *split(char *str, int ch, char *buf, int buflen, int *result);
int fix_message_file(struct session_data *sdata, struct config *cfg);

float run_statistical_check(struct session_data *sdata, struct parser_state *state, MYSQL *conn, struct config *cfg);

void manage_partitions(struct config *cfg);

void write_history_to_sql(MYSQL *conn, struct session_data *sdata, struct parser_state *state);

void store_minefield_ip(MYSQL *conn, char *ip);

int generate_tokens_from_string(struct parser_state *state, const char *s, char *label, struct config *cfg);

void digest_string(char *digestname, char *s, char *digest);

void do_training(struct session_data *sdata, struct parser_state *state, MYSQL *conn, struct config *cfg);
bool is_sender_on_minefield(MYSQL *conn, char *ip);

int check_email_against_list(MYSQL *conn, char *table, char *email);

#endif /* _KOSPAM_H */
