/*
 * avg.h, 2006.01.27, SJ
 */

// AVG ready banner

#define AVG_READY "220"

// QUIT command to AVG

#define AVG_CMD_QUIT "QUIT\r\n"

// AVG responses to the SCAN command

#define AVG_RESP_OK "200"
#define AVG_RESP_VIRUS "403"
#define AVG_RESP_NOT_FOUND "404"
#define AVG_RESP_ERROR "501"

#define AVG_OK 200
#define AVG_VIRUS 403
#define AVG_NOT_FOUND 404
#define AVG_ERROR 501

int avg_scan(char *avg_address, int avg_port, char *workdir, char *tmpdir, char *tmpfile, int v, char *avginfo);

