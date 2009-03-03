/*
 * smtpcodes.h, 2009.03.03, SJ
 */

// SMTP states

#define SMTP_STATE_INIT 0
#define SMTP_STATE_HELO 1
#define SMTP_STATE_MAIL_FROM 2
#define SMTP_STATE_RCPT_TO 3
#define SMTP_STATE_DATA 4
#define SMTP_STATE_PERIOD 5
#define SMTP_STATE_QUIT 6
#define SMTP_STATE_FINISHED 7

// SMTP commands

#define SMTP_CMD_HELO "HELO"
#define SMTP_CMD_EHLO "EHLO"
#define SMTP_CMD_MAIL_FROM "MAIL FROM:"
#define SMTP_CMD_RCPT_TO "RCPT TO:"
#define SMTP_CMD_DATA "DATA"
#define SMTP_CMD_PERIOD "\x0d\x0a\x2e\x0d\x0a"
#define SMTP_CMD_QUIT "QUIT"
#define SMTP_CMD_RESET "RSET"
#define SMTP_CMD_NOOP "NOOP"
#define SMTP_CMD_XFORWARD "XFORWARD"
#define SMTP_CMD_XCLIENT "XCLIENT"

// SMTP responses

#define SMTP_RESP_220_BANNER "220 ESMTP %s\r\n"
#define SMTP_RESP_221_GOODBYE "221 %s Goodbye\r\n"
#define SMTP_RESP_250_OK "250 Ok\r\n"
#define SMTP_RESP_250_EXTENSIONS "250-%s\r\n250-SIZE\r\n250-8BITMIME\r\n250 XFORWARD NAME ADDR PROTO HELO\r\n"
//#define SMTP_RESP_250_EXTENSIONS "250-%s\r\n250-PIPELINING\r\n250-SIZE\r\n250-8BITMIME\r\n250 XFORWARD NAME ADDR PROTO HELO\r\n"

#define SMTP_RESP_354_DATA_OK "354 Send mail data; end it with <CRLF>.<CRLF>\r\n"

#define SMTP_RESP_421_ERR "421 %s Error: timed out\r\n"
#define SMTP_RESP_421_ERR_TMP "421 %s service not available\r\n"
#define SMTP_RESP_421_ERR_WRITE_FAILED "421 writing queue file failed\r\n"
#define SMTP_RESP_450_ERR_CMD_NOT_IMPLEMENTED "450 command not implemented\r\n"
#define SMTP_RESP_451_ERR "451 Error in processing, try again later\r\n"

#define SMTP_RESP_502_ERR "502 Command not implemented\r\n"
#define SMTP_RESP_503_ERR "503 Bad command sequence\r\n"
#define SMTP_RESP_550_ERR "550 Access denied.\r\n"
#define SMTP_RESP_550_ERR_PREF "550 Access denied."
#define SMTP_RESP_552_ERR "552 Too many recipients\r\n"


// LMTP commands

#define LMTP_CMD_LHLO "LHLO"

// LMTP responses

#define LMTP_RESP_220_BANNER "220 %s LMTP\r\n"

