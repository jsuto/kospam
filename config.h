/*
 * config.h, 2007.07.09, SJ
 */

#include <syslog.h>
#include "clapf-config.h"

#define VERSION "0.3.29"

#define HOSTID "av-engine.localhost"

#define CONFIG_FILE CONFDIR "/clapf.conf"

#define LISTEN_ADDR "127.0.0.1"
#define LISTEN_PORT 10025

#define POSTFIX_ADDR "127.0.0.1"
#define POSTFIX_PORT 10026

#define AVG_ADDR "127.0.0.1"
#define AVG_PORT 55555

#define AVAST_ADDR "127.0.0.1"
#define AVAST_PORT 5036

#define KAV_SOCKET "/var/run/aveserver"

#define DRWEB_SOCKET "/var/drweb/run/.daemon"

#define CLAMD_SOCKET "/tmp/clamd"


#define QCACHE_PROGRAM_NAME "Qcache"
#define QCACHE_ADDR "127.0.0.1"
#define QCACHE_PORT 48791
#define QCACHE_SOCKET "/tmp/qcache"


#define WORK_DIR "/opt/av"
#define QUARANTINELEN 255
#define PROGNAME "clapf"
#define BACKLOG 20
#define MAXCONN 30
#define TIMEOUT 60
#define TIMEOUT_USEC 500000
#define SESSION_TIMEOUT 60
#define MAXBUFSIZE 8192
#define SMALLBUFSIZE 512
#define RANDOM_POOL "/dev/urandom"
#define RND_STR_LEN 32
#define BUFLEN 32
#define IPLEN 16

#define MAX_THREADS 30

/* this should be at least header_size_limit, see the output of postconf, 2006.08.21, SJ */
#define MAX_MAIL_HEADER_SIZE 128000

#define CLAPFUSAGE "usage: " PROGNAME " -c <config file> -d -V -h"

#define TRAINGPROGNAME "train"
#define TRAINUSAGE "usage: " TRAINGPROGNAME " -c <config file> [ <-S spam message> | <-H ham message> ]"

#define ADMINPROGNAME "clapf_admin"
#define ADMINUSAGE "usage: " ADMINPROGNAME " [-c <config file>] -u <username> -e <email address> [-a <action>] [-i <uid>]"

#define LOG_PRIORITY LOG_INFO

// logging levels

#define _LOG_DEBUG 3

#define MAX_RCPT_TO 128

// antispam stuff

#define FREQ_MIN 5

#define DEVIATION(n) fabs((n)-0.5f)

#define MAX_PHRASES_TO_CHOOSE 15
#define MAX_TOKENS_TO_CHOOSE 15

#define DEFAULT_SPAMICITY 0.5
#define DEFAULT_SPAMICITY_LOW 0.4999
#define DEFAULT_SPAMICITY_HIGH 0.5001

#define REAL_HAM_TOKEN_PROBABILITY 0.0001
#define REAL_SPAM_TOKEN_PROBABILITY 0.9999
#define MOST_INTERESTING_DEVIATION 0.4998
#define INVALID_JUNK_LIMIT 5
#define INVALID_JUNK_LINE 1
#define INVALID_HEX_JUNK_LIMIT 40
#define MAX_JUNK_SPAMICITY 0.15
#define MIN_WORD_LEN 3
/*#define MAX_WORD_LEN 19*/
#define MAX_WORD_LEN 25
#define MAX_TOKEN_LEN 8*MAX_WORD_LEN
#define DELIMITER ' '
#define TOKENSCDB WORK_DIR "/tokens.cdb"
#define SPAMINESS_HEADER_FIELD "X-Clapf-spamicity: "
#define BOUNDARY_LEN 255
#define MAX_NUM_OF_SAMPLES 65535
#define MIN_PHRASE_NUMBER 20
#define MAX_TOKEN_HASH 74713
#define MAX_HASH_STR_LEN 64
#define JUNK_REPLACEMENT_CHAR 'j'
#define DATE_STR_LEN 15
#define MESSAGES_PER_ONE_PAGE 25
#define EXCLUSION_RADIUS 0.1
#define TUM_LIMIT 25
#define NUMBER_OF_GOOD_FROM 10
#define MAX_ATTACHMENTS 8
#define MAX_ITERATIVE_TRAIN_LOOPS 5

#define MAX_CGI_SUBJECT_LEN 50
#define MAX_CGI_FROM_LEN 50

// SURBL stuff

#define SURBL_DOMAIN "multi.surbl.org"

// SQL stuff

#define SQL_TOKEN_TABLE "t_token"
#define SQL_MISC_TABLE "t_misc"
#define SQL_USER_TABLE "user"
#define SQL_TRAININGLOG_TABLE "t_train_log"
#define SQL_QUEUE_TABLE "t_queue"
#define SQL_STAT_TABLE "t_stat"

// libclamav variables

#define MAXFILES 100
#define MAX_ARCHIVED_FILE_SIZE 30*1048576
#define MAX_RECURSION_LEVEL 5
#define MAX_COMPRESS_RATIO 200
#define ARCHIVE_MEM_LIMIT 0 //disable memory limit for bzip2 scanner

// Error codes

#define OK 0
#define ERR 1

#define ERR_INJECT 20
#define ERR_REJECT 21
#define ERR_MOVED 23
#define ERR_VIRUS 24
#define ERR_DROP_SPAM 27

#define ERR_BAYES_NO_SPAM_FILE 22
#define ERR_BAYES_NO_TOKEN_FILE 25
#define ERR_BAYES_OPEN_SPAM_FILE 26
#define ERR_STAT_SPAM_FILE 28
#define ERR_BAYES_MMAP 29


#define AVIR_OK 0
#define AVIR_VIRUS 1

// passmail

#define JUNK_FOLDER "mail/junk"

#define ACTION_QUARANTINE "quarantine"
#define ACTION_JUNK "junk"
#define ACTION_DROP "drop"

#define A_QUARANTINE 0
#define A_JUNK 1
#define A_DROP 2

// training modes

#define T_TOE 0
#define T_TUM 1

// group types

#define GROUP_SHARED 0
#define GROUP_MERGED 1
