/*
 * av.h, 2008.03.12, SJ
 */

#define AV_OK 200
#define AV_VIRUS 403
#define AV_ERROR 501


// clamd stuff

#define CLAMD_RESP_CLEAN "OK"
#define CLAMD_RESP_INFECTED "FOUND"
#define CLAMD_RESP_ERROR "ERROR"

int clamd_scan(char *clamav_socket, char *chrootdir, char *workdir, char *tmpfile, int v, char *clamdinfo);
int clamd_net_scan(char *clamd_address, int clamd_port, char *chrootdir, char *workdir, char *tmpfile, int v, char *clamdinfo);

// Dr.Web stuff

#define DRWEB_RESP_VIRUS 0x20
#define DRWEB_VIRUS_HAS_FOUND_MESSAGE "Virus has been found in message. See drwebd.log for details"

int drweb_scan(char *drweb_socket, char *tmpfile, int v, char *drwebinfo);


// avast! stuff

#define AVAST_READY "220"
#define AVAST_CMD_QUIT "QUIT\r\n"

#define AVAST_RESP_OK "200"
#define AVAST_RESP_ENGINE_ERROR "451"
#define AVAST_RESP_SYNTAX_ERROR "501"

#define AVAST_RESP_CLEAN "[+]"
#define AVAST_RESP_INFECTED "[L]"

int avast_scan(char *avast_address, int avast_port, char *workdir, char *tmpfile, int v, char *avastinfo);

// Kaspersky stuff

#define KAV_CMD_QUIT "QUIT\r\n"

#define KAV_READY "201 "
#define KAV_RESP_CLEAN "220 File is clean"
#define KAV_RESP_INFECTED "230 File is infected"
#define KAV_RESP_INFECTED_NAME "322-"
#define KAV_RESP_NOT_FOUND "525 File not found"

int kav_scan(char *kav_socket, char *workdir, char *tmpfile, int v, char *kavinfo);


// avg stuff

#define AVG_READY "220"
#define AVG_CMD_QUIT "QUIT\r\n"

#define AVG_RESP_OK "200"
#define AVG_RESP_VIRUS "403"
#define AVG_RESP_NOT_FOUND "404"
#define AVG_RESP_ERROR "501"

#define AVG_NOT_FOUND 404

int avg_scan(char *avg_address, int avg_port, char *workdir, char *tmpdir, char *tmpfile, int v, char *avginfo);

