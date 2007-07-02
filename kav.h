/*
 * kav.h, 2006.02.21, SJ
 */

#define KAV_CMD_QUIT "QUIT\r\n"

// KAV responses

#define KAV_READY "201 "
#define KAV_RESP_CLEAN "220 File is clean"
#define KAV_RESP_INFECTED "230 File is infected"
#define KAV_RESP_INFECTED_NAME "322-"
#define KAV_RESP_NOT_FOUND "525 File not found"


#define KAV_OK 200
#define KAV_VIRUS 403
#define KAV_ERROR 501

int kav_scan(char *kav_socket, char *workdir, char *tmpfile, int v, char *kavinfo);
