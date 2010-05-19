/*
 * clapf.h, 2010.05.19, SJ
 */

#ifndef _CLAPF_H
 #define _CLAPF_H

#include <misc.h>
#include <list.h>
#include <bayes.h>
#include <errmsg.h>
#include <messages.h>
#include <sql.h>
#include <smtpcodes.h>
#include <session.h>
#include <buffer.h>
#include <users.h>
#include <policy.h>
#include <templates.h>
#include <hash.h>
#include <boundary.h>
#include <config.h>

#ifdef HAVE_MEMCACHED
   #include "memc.h"
#endif

int updateTokenTimestamps(struct session_data *sdata, struct node *xhash[]);

#ifdef HAVE_ANTIVIRUS
#ifdef HAVE_LIBCLAMAV
   int do_av_check(struct session_data *sdata, char *email, char *email2, char *virusinfo, struct cl_engine *engine, struct __config *cfg);
#else
   int do_av_check(struct session_data *sdata, char *email, char *email2, char *virusinfo, struct __config *cfg);
#endif
#endif

void get_queue_path(struct session_data *sdata, char **path);
void do_training(struct session_data *sdata, struct _state *state, char *email, char *acceptbuf, struct __config *cfg);
void saveMessageToQueue(struct session_data *sdata, float spaminess, struct __config *cfg);
int isSenderOnBlackOrWhiteList(struct session_data *sdata, char *email,  char *fieldname, char *table, struct __config *cfg);

int spamc_emul(char *tmpfile, int size, struct __config *cfg);

char *check_lang(struct node *xhash[]);

int store_minefield_ip(struct session_data *sdata, struct __config *cfg);
void is_sender_on_minefield(struct session_data *sdata, char *ip, struct __config *cfg);

int processMessage(struct session_data *sdata, struct _state *sstate, struct __data *data, char *email, char *email2, struct __config *cfg, struct __config *my_cfg);

struct __config read_config(char *configfile);
void print_config_all(struct __config *cfg, char *key);
void print_config(char *configfile, struct __config *cfg);

void check_dirs(struct __config *cfg, uid_t uid, gid_t gid);

int getUserdataFromMemcached(struct session_data *sdata, struct __data *data, char *email, struct __config *cfg);
int putUserdataToMemcached(struct session_data *sdata, struct __data *data, char *email, struct __config *cfg);
int getPolicyFromMemcached(struct session_data *sdata, struct __data *data, struct __config *cfg, struct __config *my_cfg);
int putPolicyToMemcached(struct session_data *sdata, struct __data *data, struct __config *cfg);

void updateCounters(struct session_data *sdata, struct __data *data, struct __counters *counters, struct __config *cfg);


#endif /* _CLAPF_H */

