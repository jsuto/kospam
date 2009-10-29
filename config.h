/*
 * config.h, 2009.10.29, SJ
 */

#ifndef _CONFIG_H
 #define _CONFIG_H

#include <syslog.h>
#include "clapf-config.h"

#define PROGNAME "clapf"

#define VERSION "0.4.3-rc1"

#define HOSTID "av-engine.localhost"

#define CONFIG_FILE CONFDIR "/clapf.conf"
#define USER_DATA_DIR DATADIR "/lib/clapf/data"
#define USER_QUEUE_DIR DATADIR "/lib/clapf/queue"
#define PER_USER_SQLITE3_DB_FILE "clapf.sdb"
#define MYDB_FILE "tokens.mydb"

#define QUEUE_DIR_PERMISSION 0755

#define SQLITE3_PRAGMA "PRAGMA synchronous = OFF"

#define LISTEN_ADDR "127.0.0.1"
#define LISTEN_PORT 10025

#define POSTFIX_ADDR "127.0.0.1"
#define POSTFIX_PORT 10026

#define AVAST_ADDR "127.0.0.1"
#define AVAST_PORT 5036

#define KAV_SOCKET "/var/run/aveserver"

#define DRWEB_SOCKET "/var/drweb/run/.daemon"

#define CLAMD_ADDR "127.0.0.1"
#define CLAMD_PORT 3310
#define CLAMD_SOCKET "/tmp/clamd"


#define WORK_DIR DATADIR "/clapf"
#define PIDFILE WORK_DIR "/clapf.pid"
#define QUARANTINELEN 255
#define BACKLOG 20
#define MAXCONN 30
#define TIMEOUT 60
#define TIMEOUT_USEC 500000
#define SESSION_TIMEOUT 420
#define MAXBUFSIZE 8192
#define SMALLBUFSIZE 512
#define RANDOM_POOL "/dev/urandom"
#define RND_STR_LEN 32
#define MESSAGE_ID_LEN 32
#define BUFLEN 32
#define IPLEN 16+1

#define MYDB_MIN_SIZE 1000000

#define MEMCACHED_KEY_NAME "clapf:pos"
#define MEMCACHED_KEY_LENGTH 9
#define MEMCACHED_MESSAGE_NAME "clapf_msg:"

/* this should be at least header_size_limit, see the output of postconf, 2006.08.21, SJ */
#define MAX_MAIL_HEADER_SIZE 128000

#define MAILBUFSIZE 32000

#define CLAPFUSAGE "usage: " PROGNAME " -c <config file> -u <uid> -g <gid> -d -V -h"

#define SPAMDROPUSAGE "usage: spamdrop [-c <config file>] [-u <username>] [-s] [-p] [-H | -S] < message"


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
#define DEFAULT_SPAMICITY_LOW 0.4999
#define DEFAULT_SPAMICITY_HIGH 0.5001

#define REAL_HAM_TOKEN_PROBABILITY 0.0001
#define REAL_SPAM_TOKEN_PROBABILITY 0.9999
#define MOST_INTERESTING_DEVIATION 0.4998
#define INVALID_JUNK_LIMIT 5
#define INVALID_JUNK_LINE 1
#define INVALID_HEX_JUNK_LIMIT 40
#define MAX_HAM_SPAMICITY 0.39
#define MIN_WORD_LEN 3
#define MAX_WORD_LEN 25
#define MAX_TOKEN_LEN 4*MAX_WORD_LEN
#define URL_LEN 48
#define DELIMITER ' '
#define SPAMINESS_HEADER_FIELD "X-Clapf-spamicity: "
#define BOUNDARY_LEN 255
#define MAX_NUM_OF_SAMPLES 65535
#define MIN_PHRASE_NUMBER 20
#define JUNK_REPLACEMENT_CHAR 'j'
#define DATE_STR_LEN 15
#define EXCLUSION_RADIUS 0.375
#define NUMBER_OF_GOOD_FROM 10
#define MAX_ATTACHMENTS 8
#define MAX_ITERATIVE_TRAIN_LOOPS 5
#define NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED 1000

#define ROLLING_TOKENS 4096

#define MAX_CGI_SUBJECT_LEN 50
#define MAX_CGI_FROM_LEN 50

/* SURBL stuff */

#define SURBL_DOMAIN "multi.surbl.org"

/* SQL stuff */

#define SQL_TOKEN_TABLE "t_token"
#define SQL_MISC_TABLE "t_misc"
#define SQL_USER_TABLE "user"
#define SQL_EMAIL_TABLE "t_email"
#define SQL_WHITE_LIST "t_white_list"
#define SQL_BLACK_LIST "t_black_list"
#define SQL_QUEUE_TABLE "t_queue"
#define SQL_STAT_TABLE "t_stat"
#define SQL_POLICY_TABLE "t_policy"
#define SQL_MINEFIELD_TABLE "t_minefield"

#define SQL_WHITE_FIELD_NAME "whitelist"
#define SQL_BLACK_FIELD_NAME "blacklist"

#define _90_DAYS 90*86400
#define _60_DAYS 60*86400
#define _30_DAYS 30*86400
#define _15_DAYS 15*86400

/* libclamav variables */

#define MAXFILES 100
#define MAX_ARCHIVED_FILE_SIZE 30*1048576
#define MAX_RECURSION_LEVEL 5
#define MAX_COMPRESS_RATIO 200
#define ARCHIVE_MEM_LIMIT 0 //disable memory limit for bzip2 scanner


/* TRE stuff */

#define NUM_OF_REGEXES 16


/* Error codes */

#define OK 0
#define ERR 1

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

