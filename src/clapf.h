/*
 * clapf.h, SJ
 */

#ifndef _CLAPF_H
 #define _CLAPF_H

#include <misc.h>
#include <list.h>
#include <parser.h>
#include <bayes.h>
#include <messages.h>
#include <smtpcodes.h>
#include <session.h>
#include <buffer.h>
#include <decoder.h>
#include <users.h>
#include <policy.h>
#include <score.h>
#include <templates.h>
#include <hash.h>
#include <boundary.h>
#include <defs.h>
#include <sig.h>
#include <rbl.h>
#include <av.h>
#include <chi.h>
#include <score.h>
#include <config.h>

#ifdef HAVE_MEMCACHED
   #include "memc.h"
#endif

struct te getHamSpamCounters(struct session_data *sdata, char *stmt);
int update_hash(struct session_data *sdata, char *qry, struct node *xhash[]);

int introduceTokens(struct session_data *sdata, struct node *xhash[]);
int updateTokenCounters(struct session_data *sdata, int ham_or_spam, struct node *xhash[], int train_mode);
int updateMiscTable(struct session_data *sdata, int ham_or_spam, int train_mode);
int updateTokenTimestamps(struct session_data *sdata, struct node *xhash[]);

int do_av_check(struct session_data *sdata, char *rcpttoemail, char *fromemail, char *virusinfo, struct __data *data, struct __config *cfg);

void get_queue_path(struct session_data *sdata, char **path, struct __config *cfg);
void do_training(struct session_data *sdata, struct _state *state, char *email, char *acceptbuf, struct __config *cfg);
void saveMessageToQueue(struct session_data *sdata, float spaminess, struct __config *cfg);
void getWBLData(struct session_data *sdata, struct __config *cfg);

int spamc_emul(char *tmpfile, int size, struct __config *cfg);

char *check_lang(struct node *xhash[]);

int store_minefield_ip(struct session_data *sdata, struct __config *cfg);
void is_sender_on_minefield(struct session_data *sdata, char *ip, struct __config *cfg);

int processMessage(struct session_data *sdata, struct _state *sstate, struct __data *data, char *rcpttoemail, char *fromemail, struct __config *cfg, struct __config *my_cfg);

struct __config read_config(char *configfile);

void checkAndCreateClapfDirectories(struct __config *cfg, uid_t uid, gid_t gid);

int getUserdataFromMemcached(struct session_data *sdata, struct __data *data, char *email, struct __config *cfg);
int putUserdataToMemcached(struct session_data *sdata, struct __data *data, char *email, struct __config *cfg);
int getPolicyFromMemcached(struct session_data *sdata, struct __data *data, struct __config *cfg, struct __config *my_cfg);
int putPolicyToMemcached(struct session_data *sdata, struct __data *data, struct __config *cfg);
int getWBLFromMemcached(struct session_data *sdata, struct __data *data, struct __config *cfg);
int putWBLToMemcached(struct session_data *sdata, struct __data *data, struct __config *cfg);

void getUserFromEmailAddress(struct session_data *sdata, struct __data *data, char *email, struct __config *cfg);
void getPolicySettings(struct session_data *sdata, struct __data *data, struct __config *cfg, struct __config *my_cfg);
void getUsersWBL(struct session_data *sdata, struct __data *data, struct __config *cfg);
void checkZombieSender(struct session_data *sdata, struct __data *data, struct _state *state, struct __config *cfg);

void updateCounters(struct session_data *sdata, struct __data *data, struct __counters *counters, struct __config *cfg);

void initialiseZombieList(struct __data *data, struct __config *cfg);
void freeZombieList(struct __data *data);

#endif /* _CLAPF_H */

