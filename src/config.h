/*
 * config.h, SJ
 */

#ifndef _CONFIG_H
 #define _CONFIG_H

#include <syslog.h>
#include "clapf-config.h"
#include "params.h"

#define PROGNAME "clapf"

#define VERSION "nightly-20111113"

#define PROGINFO VERSION ", Janos SUTO <sj@acts.hu>\n\n" CONFIGURE_PARAMS "\n\nSend bugs/issues to https://jira.acts.hu:8443/\n"

#define HOSTID "av-engine.localhost"

#define CONFIG_FILE CONFDIR "/clapf.conf"
#define USER_DATA_DIR LOCALSTATEDIR "/lib/clapf/data"
#define USER_QUEUE_DIR LOCALSTATEDIR "/lib/clapf/queue"
#define WORK_DIR LOCALSTATEDIR "/spool/clapf/tmp"
#define PER_USER_SQLITE3_DB_FILE "tokens.sdb"
#define MYDB_FILE "tokens.mydb"

#define CLAMD_SOCKET "/tmp/clamd"
#define DRWEB_SOCKET "/var/drweb/run/.daemon"
#define KAV_SOCKET "/var/run/aveserver"

#define QUARANTINELEN 255
#define TIMEOUT 60
#define TIMEOUT_USEC 500000
#define SESSION_TIMEOUT 420
#define MAXBUFSIZE 8192
#define SMALLBUFSIZE 512
#define MAXVAL 256
#define RANDOM_POOL "/dev/urandom"
#define RND_STR_LEN 32
#define MESSAGE_ID_LEN 32
#define BUFLEN 32
#define IPLEN 16+1

#define CRLF "\n"

#define MYDB_MIN_SIZE 1000000

#define MEMCACHED_CLAPF_PREFIX "_c"
#define MAX_MEMCACHED_KEY_LEN 250

#define MEMCACHED_SUCCESS 0
#define MEMCACHED_FAILURE 1

#define MEMCACHED_COUNTERS_LAST_UPDATE MEMCACHED_CLAPF_PREFIX ":counters_last_update"
#define MEMCACHED_MSGS_RCVD MEMCACHED_CLAPF_PREFIX ":rcvd"
#define MEMCACHED_MSGS_MYNETWORK MEMCACHED_CLAPF_PREFIX ":mynetwork"
#define MEMCACHED_MSGS_HAM MEMCACHED_CLAPF_PREFIX ":ham"
#define MEMCACHED_MSGS_SPAM MEMCACHED_CLAPF_PREFIX ":spam"
#define MEMCACHED_MSGS_POSSIBLE_SPAM MEMCACHED_CLAPF_PREFIX ":possible_spam"
#define MEMCACHED_MSGS_UNSURE MEMCACHED_CLAPF_PREFIX ":unsure"
#define MEMCACHED_MSGS_MINEFIELD MEMCACHED_CLAPF_PREFIX ":minefield"
#define MEMCACHED_MSGS_ZOMBIE MEMCACHED_CLAPF_PREFIX ":zombie"
#define MEMCACHED_MSGS_VIRUS MEMCACHED_CLAPF_PREFIX ":virus"
#define MEMCACHED_MSGS_FP MEMCACHED_CLAPF_PREFIX ":fp"
#define MEMCACHED_MSGS_FN MEMCACHED_CLAPF_PREFIX ":fn"


/* this should be at least header_size_limit, see the output of postconf, 2006.08.21, SJ */
#define MAX_MAIL_HEADER_SIZE 128000

#define MAILBUFSIZE 32000

#define CLAPFUSAGE "usage: " PROGNAME " -c <config file> -d -V -h"

#define SPAMDROPUSAGE "usage: spamdrop [-c <config file>] [-u <username> | -U uid] [-f from] [-r recipient] [-dsp] [-H | -S | -D] < message"

#define LOG_PRIORITY LOG_INFO

// logging levels

#define _LOG_INFO 3
#define _LOG_DEBUG 5

#define MAX_RCPT_TO 128

// antispam stuff

#define FREQ_MIN 5

#define DEVIATION(n) fabs((n)-0.5f)


#ifdef HAVE_SQLITE3
   #define MAX_KEY_VAL  9223372036854775807ULL
#else
   #define MAX_KEY_VAL 18446744073709551615ULL
#endif

#define DEFAULT_SPAMICITY 0.5

#define REAL_HAM_TOKEN_PROBABILITY 0.0001
#define REAL_SPAM_TOKEN_PROBABILITY 0.9999
#define MIN_WORD_LEN 3
#define MAX_WORD_LEN 25
#define MAX_TOKEN_LEN 4*MAX_WORD_LEN
#define URL_LEN 48
#define DELIMITER ' '
#define SPAMINESS_HEADER_FIELD "X-Clapf-spamicity: "
#define BOUNDARY_LEN 255
#define MAX_NUM_OF_SAMPLES 65535
#define JUNK_REPLACEMENT_CHAR 'j'
#define NUMBER_OF_GOOD_FROM 10
#define MAX_ATTACHMENTS 8
#define MAX_ITERATIVE_TRAIN_LOOPS 5
#define NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED 1000

/* SQL stuff */

#define SQL_TOKEN_TABLE "t_token"
#define SQL_MISC_TABLE "t_misc"
#define SQL_USER_TABLE "user"
#define SQL_EMAIL_TABLE "t_email"
#define SQL_WHITE_LIST "t_white_list"
#define SQL_BLACK_LIST "t_black_list"
#define SQL_QUEUE_TABLE "t_queue"
#define SQL_POLICY_TABLE "t_policy"
#define SQL_MINEFIELD_TABLE "t_minefield"

#define SQL_WHITE_FIELD_NAME "whitelist"
#define SQL_BLACK_FIELD_NAME "blacklist"

#define _90_DAYS 90*86400
#define _60_DAYS 60*86400
#define _30_DAYS 30*86400
#define _15_DAYS 15*86400


/* TRE stuff */

#define NUM_OF_REGEXES 20


/* Error codes */

#define OK 0
#define ERR 1

#define DISCARD 1

#define ERR_INJECT 20
#define ERR_REJECT 21
#define ERR_DROP_SPAM 27


#define AVIR_OK 0
#define AVIR_VIRUS 1



/* training modes */

#define T_TOE 0
#define T_TUM 1

/* group types */

#define GROUP_SHARED 0
#define GROUP_MERGED 1


#endif /* _CONFIG_H */

