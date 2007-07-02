/*
 * avast.h, 2006.02.07, SJ
 */

// AVAST ready banner

#define AVAST_READY "220"

// QUIT command to AVAST

#define AVAST_CMD_QUIT "QUIT\r\n"

// AVAST responses to the SCAN command

#define AVAST_RESP_OK "200"
#define AVAST_RESP_ENGINE_ERROR "451"
#define AVAST_RESP_SYNTAX_ERROR "501"

#define AVAST_RESP_CLEAN "[+]"
#define AVAST_RESP_INFECTED "[L]"

#define AVAST_OK 200
#define AVAST_VIRUS 403
#define AVAST_ERROR 501

int avast_scan(char *avast_address, int avast_port, char *workdir, char *tmpfile, int v, char *avastinfo);

