/*
 * clamd.h, 2006.03.09, SJ
 */

// CLAMD responses

#define CLAMD_RESP_CLEAN "OK"
#define CLAMD_RESP_INFECTED "FOUND"
#define CLAMD_RESP_ERROR "ERROR"

#define CLAMD_OK 200
#define CLAMD_VIRUS 403
#define CLAMD_ERROR 501

int clamd_scan(char *clamav_socket, char *workdir, char *tmpfile, int v, char *clamdinfo);

