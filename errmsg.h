/*
 * errmsg.h, 2008.04.05, SJ
 */

#ifndef _ERRMSG_H
 #define _ERRMSG_H

// Error messages

#define ERR_CANNOT_READ_FROM_POOL "ERR: cannot read from pool"
#define ERR_SIGACTION "sigaction failed"
#define ERR_OPEN_SOCKET "ERR: cannot open socket"
#define ERR_SET_SOCK_OPT "ERR: cannot set socket option"
#define ERR_BIND_TO_PORT "ERR: cannot bind to port"
#define ERR_LISTEN "ERR: cannot listen"
#define ERR_SELECT "ERR: select()"
#define ERR_CHDIR "ERR: chdir() to working directory failed"
#define ERR_LOAD_DB "ERR: loading database"
#define ERR_RELOAD_DB "ERR: reloading database"
#define ERR_OPEN_TMP_FILE "ERR: opening a tempfile"
#define ERR_TIMED_OUT "ERR: timed out"
#define ERR_TOKEN_INIT "ERR: token_init() failed"
#define ERR_FORK_FAILED "ERR: cannor fork()"

// clamav error messages

#define ERR_STAT_INI_DIR "ERR: cl_statinidir()"
#define ERR_NO_DB_DIR "ERR: no clamav database dir found"
#define ERR_BUILD_TRIE "ERR: building trie"
#define ERR_STATCHKDIR "ERR: halt because of cl_statchkdir()"

#endif /* _ERRMSG_H */
