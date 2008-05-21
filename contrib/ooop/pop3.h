/*
 * pop3.h, 2008.05.18, SJ
 */

// POP3 states

#define POP3_STATE_INIT 0
#define POP3_STATE_USER 1
#define POP3_STATE_PASS 2
#define POP3_STATE_QUIT 3
#define POP3_STATE_FINISHED 4

// POP3 commands

#define POP3_CMD_AUTH "AUTH"
#define POP3_CMD_USER "USER "
#define POP3_CMD_PASS "PASS "
#define POP3_CMD_HELP "HELP"
#define POP3_CMD_STAT "STAT"
#define POP3_CMD_LIST "LIST"
#define POP3_CMD_RETR "RETR"
#define POP3_CMD_NOOP "NOOP"
#define POP3_CMD_DELE "DELE"
#define POP3_CMD_RSET "RSET"
#define POP3_CMD_TOP  "TOP"
#define POP3_CMD_UIDL "UIDL"
#define POP3_CMD_LAST "LAST"
#define POP3_CMD_QUIT "QUIT"
#define POP3_CMD_CAPA "CAPA"

// POP3 responses

#define POP3_RESP_BANNER "+OK POP3 server ready\r\n"
#define POP3_RESP_SEND_PASS "+OK please send the PASS\r\n"
#define POP3_RESP_HELP "+OK Valid commands: USER, PASS, STAT, LIST, RETR, DELE, RSET, NOOP, UIDL, QUIT, HELP\r\n"
#define POP3_RESP_CAPA "+OK Capability list follows\r\nTOP\r\nUIDL\r\n.\r\n"
#define POP3_RESP_QUIT "+OK POP3 server connection closed\r\n"
#define POP3_RESP_OK "+OK\r\n"

#define POP3_RESP_INVALID_AUTH_TYPE "-ERR Unrecognized authentication type\r\n"
#define POP3_RESP_INVALID_PWD "-ERR Incorrect password or account name\r\n"
#define POP3_RESP_INVALID_CMD "-ERR Invalid command; use HELP\r\n"
#define POP3_RESP_INVALID_LIST_ARG "-ERR Invalid argument with LIST\r\n"
#define POP3_RESP_INVALID_RETR_ARG "-ERR Invalid argument with RETR\r\n"
#define POP3_RESP_INTERNAL_ERROR "-ERR Internal error\r\n"
#define POP3_RESP_NO_SUCH_MESSAGE "-ERR no such message\r\n"

