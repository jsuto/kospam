/*
 * cgi.h, 2008.02.13, SJ
 */

#include "config.h"

#define M_UNDEF 0
#define M_GET 1
#define M_POST 2


struct cgidata {
   char id[SMALLBUFSIZE];
   char delivery[SMALLBUFSIZE];
   char train[SMALLBUFSIZE];
   char user[SMALLBUFSIZE];
   char type[SMALLBUFSIZE];
   int page;
};


void errout(char *input, char *s);
struct cgidata extract_cgi_parameters(char *data);
void show_users(char *queuedir, char *spamcgi_url);
void scan_message(char *dir, char *message, char *from, char *subj);
int check_directory(char *dir, char *username, int page, int page_len, char *spamcgi_url);
void show_message(char *dir, char *message);
