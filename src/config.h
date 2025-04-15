/*
 * config.h, SJ
 */

#ifndef _CONFIG_H
 #define _CONFIG_H

#include <syslog.h>
#include "kospam-config.h"
#include "params.h"

#define HOSTNAME "kospam.localhost"

#define CONFIG_FILE CONFDIR "/kospam/kospam.conf"
#define WORK_DIR DATADIR "/kospam/tmp"
#define HISTORY_DIR DATADIR "/kospam/history"
#define SEND_DIR DATADIR "/kospam/send"
#define QUARANTINE_DIR DATADIR "/kospam/quarantine"
#define ZOMBIE_NET_REGEX CONFDIR "/kospam/zombienets.regex"

#define SPAMINESS_HEADER_FIELD "X-Clapf-spamicity: "
#define SPAM_HEADER_FIELD SPAMINESS_HEADER_FIELD "Yes"

#define PIDFILE "/var/kospam/run/kospam.pid"
#define QUARANTINELEN 255
#define MAXBUFSIZE 8192
#define SMALLBUFSIZE 512
#define BIGBUFSIZE 131072
#define TINYBUFSIZE 128
#define MAXVAL 256
#define BUFLEN 32
#define MAX_MAIL_HEADER_SIZE 128000


#define LOG_PRIORITY LOG_INFO

#define _LOG_INFO 3
#define _LOG_DEBUG 5

#define MAX_ATTACHMENTS 16

/* SQL stuff */

#define SQL_COUNTER_TABLE "counter"
#define SQL_OPTION_TABLE "option"
#define SQL_DOMAIN_TABLE "domain"
#define SQL_USER_TABLE "user"
#define SQL_EMAIL_TABLE "email"
#define SQL_MISC_TABLE "misc"
#define SQL_TOKEN_TABLE "token"
#define SQL_MINEFIELD_TABLE "minefield"
#define SQL_WHITE_LIST "whitelist"
#define SQL_BLACK_LIST "blacklist"
#define SQL_HISTORY_TABLE "history"
#define SQL_ATTACH_DIGEST_TABLE "attach_digest"
#define SQL_TEMP_TOKEN_TABLE "temp_token"

#define SQL_PREPARED_STMT_QUERY_MINEFIELD "SELECT ts FROM " SQL_MINEFIELD_TABLE " WHERE ip=?"
#define SQL_PREPARED_STMT_INSERT_INTO_MINEFIELD "REPLACE INTO " SQL_MINEFIELD_TABLE " (ip, ts) VALUES(?, ?)"
#define SQL_PREPARED_STMT_INSERT_INTO_HISTORY "INSERT INTO " SQL_HISTORY_TABLE " (ts, clapf_id, spam, sender, subject, size) VALUES (?,?,?,?,?,?)"
#define SQL_PREPARED_STMT_QUERY_TRAINING_ID "SELECT spam FROM " SQL_HISTORY_TABLE " WHERE clapf_id=?"
#define SQL_PREPARED_STMT_QUERY_ADIGEST "SELECT counter FROM " SQL_ATTACH_DIGEST_TABLE " WHERE digest=?"
#define SQL_PREPARED_STMT_UPDATE_ADIGEST "UPDATE " SQL_ATTACH_DIGEST_TABLE " SET counter=counter+1 WHERE digest=?"


/* Error codes */

#define OK 0
#define ERR 1

#define DISCARD 1

#define S_HAM 0
#define S_SPAM 1

#define ERR_CHDIR "ERROR: chdir() to working directory failed"
#define ERR_DAEMON "ERROR: daemon()"
#define ERR_NON_EXISTENT_USER "ERROR: non existent user in config file, see the 'username' variable"
#define ERR_SETUID "ERROR: setuid()"
#define ERR_SQL_DATA "ERROR: no valid data from sql table"

#endif /* _CONFIG_H */
