/*
 * drweb.h, 2006.02.28, SJ
 */

#define DRWEB_RESP_VIRUS 0x20

#define DRWEB_OK 200
#define DRWEB_VIRUS 403
#define DRWEB_ERROR 501

#define DRWEB_VIRUS_HAS_FOUND_MESSAGE "Virus has been found in message. See drwebd.log for details"

int drweb_scan(char *drweb_socket, char *tmpfile, int v, char *drwebinfo);
