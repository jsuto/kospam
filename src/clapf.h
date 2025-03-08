/*
 * clapf.h, SJ
 */

#ifndef _CLAPF_H
 #define _CLAPF_H

#include <misc.h>
#include <parser.h>
#include <errmsg.h>
#include <smtpcodes.h>
#include <decoder.h>
#include <buffer.h>
#include <hash.h>
#include <defs.h>
#include <tai.h>
#include <sig.h>
#include <av.h>
#include <sql.h>
#include <chi.h>
#include <users.h>
#include <config.h>
#include <unistd.h>
#include <math.h>

#ifdef HAVE_MEMCACHED
   #include "memc.h"
#endif

#define DEVIATION(n) fabs((n)-0.5f)

int do_av_check(struct session_data *sdata, char *virusinfo, struct __data *data, struct __config *cfg);
int check_for_known_bad_attachments(struct session_data *sdata, struct __state *state);

void digest_file(char *filename, char *digest);

int handle_smtp_session(int new_sd, struct __data *data, struct __config *cfg);
int handle_pilerget_request(int new_sd, struct __data *data, struct __config *cfg);

void remove_stripped_attachments(struct __state *state);
int process_message(struct session_data *sdata, struct __state *state, struct __data *data, struct __config *cfg);

struct __config read_config(char *configfile);

void check_and_create_directories(struct __config *cfg);

void update_counters(struct session_data *sdata, struct __counters *counters);
int update_token_timestamps(struct session_data *sdata, struct node *xhash[]);

int get_policy(struct session_data *sdata, struct __config *cfg, struct __config *my_cfg);

int import_message(char *filename, struct session_data *sdata, struct __data *data, struct __config *cfg);
int get_folder_id(struct session_data *sdata, struct __data *data, char *foldername, int parent_id);
int add_new_folder(struct session_data *sdata, struct __data *data, char *foldername, int parent_id);

int check_spam(struct session_data *sdata, struct __state *state, struct __data *data, char *fromemail, char *rcpttoemail, struct __config *cfg, struct __config *my_cfg);
float run_statistical_check(struct session_data *sdata, struct __state *state, struct __config *cfg);
void add_penalties(struct session_data *sdata, struct __state *state, struct __config *cfg);

int train_message(struct session_data *sdata, struct __state *state, int rounds, int is_spam, int train_mode, struct __config *cfg);
void do_training(struct session_data *sdata, struct __state *state, char *email, struct __config *cfg);

int generate_tokens_from_string(struct __state *state, char *s, char *label);
void tokenize(char *buf, struct __state *state, struct session_data *sdata, struct __config *cfg);

void zombie_init(struct __data *data, struct __config *cfg);
void check_zombie_sender(struct session_data *sdata, struct __data *data, struct __config *cfg);
void zombie_free(struct __data *data);

void store_minefield_ip(struct session_data *sdata, char *ip);
void is_sender_on_minefield(struct session_data *sdata, char *ip);

int inject_mail(struct session_data *sdata, int msg, char *spaminessbuf, char *buf, struct __config *cfg);

int write_history(struct session_data *sdata, struct __state *state, char *status, struct __config *cfg);
int create_partition(struct __config *cfg);
int drop_partition(struct __config *cfg);

int check_rbl_lists(struct __state *state, char *domainlist);

void init_child_stat_entry(struct session_data *sdata);
void create_child_stat_entry(struct session_data *sdata, pid_t pid);
void remove_child_stat_entry(struct session_data *sdata, pid_t pid);
void update_child_stat_entry(struct session_data *sdata, char status, int count);

#endif /* _CLAPF_H */

