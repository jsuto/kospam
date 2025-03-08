/*
 * config.h, SJ
 */

#ifndef _CONFIG_H
 #define _CONFIG_H

#include <syslog.h>
#include "clapf-config.h"
#include "params.h"

#define PROGNAME "clapf"

#define VERSION "0.5.2"

#define BUILD 1322

#define HOSTID "av-engine.localhost"

#define CONFIG_FILE CONFDIR "/clapf/clapf.conf"
#define WORK_DIR DATADIR "/clapf/tmp"
#define QUEUE_DIR DATADIR "/clapf/queue"
#define HISTORY_DIR DATADIR "/clapf/history"

#define SPAMINESS_HEADER_FIELD "X-Clapf-spamicity: "
#define SPAM_HEADER_FIELD SPAMINESS_HEADER_FIELD "Yes"

#define CLAMD_SOCKET "/var/run/clamav/clamd.ctl"

#define PIDFILE "/var/clapf/run/clapf.pid"
#define QUARANTINELEN 255
#define TIMEOUT 60
#define TIMEOUT_USEC 500000
#define SESSION_TIMEOUT 420
#define MAXBUFSIZE 8192
#define SMALLBUFSIZE 512
#define BIGBUFSIZE 131072
#define REALLYBIGBUFSIZE 524288
#define TINYBUFSIZE 128
#define MAXVAL 256
#define RANDOM_POOL "/dev/urandom"
#define RND_STR_LEN 36
#define BUFLEN 32
#define IPLEN 16+1
#define KEYLEN 56
#define MAX_MAIL_HEADER_SIZE 128000

#define LMTP_MODE 0
#define SMTP_MODE 1

#define CRLF "\n"


#define MEMCACHED_CLAPF_PREFIX "_clapf:"
#define MAX_MEMCACHED_KEY_LEN 250

#define MEMCACHED_SUCCESS 0
#define MEMCACHED_FAILURE 1

#define MEMCACHED_COUNTERS_LAST_UPDATE MEMCACHED_CLAPF_PREFIX "counters_last_update"
#define MEMCACHED_MSGS_RCVD MEMCACHED_CLAPF_PREFIX "rcvd"
#define MEMCACHED_MSGS_MYNETWORK MEMCACHED_CLAPF_PREFIX "mynetwork"
#define MEMCACHED_MSGS_HAM MEMCACHED_CLAPF_PREFIX "ham"
#define MEMCACHED_MSGS_SPAM MEMCACHED_CLAPF_PREFIX "spam"
#define MEMCACHED_MSGS_POSSIBLE_SPAM MEMCACHED_CLAPF_PREFIX "possible_spam"
#define MEMCACHED_MSGS_UNSURE MEMCACHED_CLAPF_PREFIX "unsure"
#define MEMCACHED_MSGS_MINEFIELD MEMCACHED_CLAPF_PREFIX "minefield"
#define MEMCACHED_MSGS_ZOMBIE MEMCACHED_CLAPF_PREFIX "zombie"
#define MEMCACHED_MSGS_VIRUS MEMCACHED_CLAPF_PREFIX "virus"
#define MEMCACHED_MSGS_FP MEMCACHED_CLAPF_PREFIX "fp"
#define MEMCACHED_MSGS_FN MEMCACHED_CLAPF_PREFIX "fn"
#define MEMCACHED_MSGS_SIZE MEMCACHED_CLAPF_PREFIX "size"

#define LOG_PRIORITY LOG_INFO

#define _LOG_INFO 3
#define _LOG_DEBUG 5

#define MAX_RCPT_TO 128

#define MIN_WORD_LEN 3
#define MAX_WORD_LEN 25
#define MAX_TOKEN_LEN 4*MAX_WORD_LEN
#define DELIMITER ' '
#define BOUNDARY_LEN 255
#define MAX_ATTACHMENTS 16
#define MAX_ZIP_RECURSION_LEVEL 2

/* SQL stuff */

#define SQL_COUNTER_TABLE "counter"
#define SQL_OPTION_TABLE "option"
#define SQL_DOMAIN_TABLE "domain"
#define SQL_USER_TABLE "user"
#define SQL_EMAIL_TABLE "email"
#define SQL_MISC_TABLE "misc"
#define SQL_TOKEN_TABLE "token"
#define SQL_MINEFIELD_TABLE "minefield"
#define SQL_WHITE_LIST "t_white_list"
#define SQL_BLACK_LIST "t_black_list"
#define SQL_POLICY_TABLE "policy"
#define SQL_HISTORY_TABLE "history"
#define SQL_STATUS_TABLE "status"
#define SQL_ATTACH_DIGEST_TABLE "attach_digest"

#define SQL_PREPARED_STMT_QUERY_MINEFIELD "SELECT ts FROM " SQL_MINEFIELD_TABLE " WHERE ip=?"
#define SQL_PREPARED_STMT_INSERT_INTO_BLACKHOLE "REPLACE INTO " SQL_MINEFIELD_TABLE " (ip, ts) VALUES(?, ?)"
#define SQL_PREPARED_STMT_QUERY_WHITE_BLACK_LIST "SELECT whitelist, blacklist FROM " SQL_WHITE_LIST "," SQL_BLACK_LIST " where (" SQL_WHITE_LIST ".uid=? OR " SQL_WHITE_LIST ".uid=0) AND " SQL_WHITE_LIST ".uid=" SQL_BLACK_LIST ".uid"
#define SQL_PREPARED_STMT_QUERY_USER_DATA "SELECT u.uid, u.gid, u.username, u.domain, u.policy_group FROM " SQL_USER_TABLE " u, " SQL_EMAIL_TABLE " e WHERE u.uid=e.uid AND e.email=?"
#define SQL_PREPARED_STMT_QUERY_POLICY "SELECT deliver_infected_email, silently_discard_infected_email, use_antispam, spam_subject_prefix, max_message_size_to_filter, surbl_domain, spam_overall_limit, spaminess_oblivion_limit, replace_junk_characters, penalize_images, penalize_embed_images, penalize_octet_stream, training_mode, store_emails, store_only_spam, message_from_a_zombie, smtp_addr, smtp_port FROM " SQL_POLICY_TABLE " WHERE id=?"
#define SQL_PREPARED_STMT_INSERT_INTO_HISTORY "INSERT INTO " SQL_HISTORY_TABLE " (ts, clapf_id, spam, sender, rcpt, subject, size, attachments, relay, status) VALUES (?,?,?,?,?,?,?,?,?,?)"
#define SQL_PREPARED_STMT_QUERY_TRAINING_ID "SELECT spam FROM " SQL_HISTORY_TABLE " WHERE clapf_id=?"
#define SQL_PREPARED_STMT_QUERY_ADIGEST "SELECT counter FROM " SQL_ATTACH_DIGEST_TABLE " WHERE digest=?"
#define SQL_PREPARED_STMT_UPDATE_ADIGEST "UPDATE " SQL_ATTACH_DIGEST_TABLE " SET counter=counter+1 WHERE digest=?"


/* Error codes */

#define OK 0
#define ERR 1
#define ERR_EXISTS 2
#define ERR_MYDOMAINS 3
#define ERR_INJECT 20
#define ERR_REJECT 21

#define AVIR_OK 0
#define AVIR_VIRUS 1

#define DISCARD 1

#define DIRECTION_INCOMING 0
#define DIRECTION_INTERNAL 1
#define DIRECTION_OUTGOING 2
#define DIRECTION_INTERNAL_AND_OUTGOING 3

#define WRITE_TO_STDOUT 0
#define WRITE_TO_BUFFER 1

#define S_STATUS_UNDEF "undef"
#define S_STATUS_SENT "sent"
#define S_STATUS_DISCARDED "discarded"
#define S_STATUS_ERROR "error"
#define S_STATUS_REJECT "reject"

#define S_HAM 0
#define S_SPAM 1
#define S_VIRUS 2


#endif /* _CONFIG_H */

