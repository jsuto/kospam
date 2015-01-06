/*
 * errmsg.h, SJ
 */

#ifndef _ERRMSG_H
 #define _ERRMSG_H

#define ERR_CANNOT_READ_FROM_POOL "error: cannot read from pool"
#define ERR_SIGACTION "sigaction failed"
#define ERR_OPEN_SOCKET "error: cannot open socket"
#define ERR_SET_SOCK_OPT "error: cannot set socket option"
#define ERR_BIND_TO_PORT "error: cannot bind to port"
#define ERR_LISTEN "error: cannot listen"
#define ERR_SETUID "error: setuid()"
#define ERR_SETGID "error: setgid()"
#define ERR_SELECT "error: select()"
#define ERR_CHDIR "error: chdir() to working directory failed"
#define ERR_OPEN_TMP_FILE "error: opening a tempfile"
#define ERR_TIMED_OUT "error: timed out"
#define ERR_FORK_FAILED "error: cannot fork()"

#define ERR_MYSQL_CONNECT "cannot connect to mysql server"
#define ERR_SQL_DATA "no valid data from sql table"

#define ERR_NON_EXISTENT_USER "error: non existent user in config file, see the 'username' variable"

#define ERR_READING_KEY "error: reading cipher key"

#endif /* _ERRMSG_H */
