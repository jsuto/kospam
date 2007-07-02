/*
 * cgi.h, 2006.10.13, SJ
 */

#define M_UNDEF 0
#define M_GET 1
#define M_POST 2

void errout(char *input, char *s);
void create_ham_or_spam_path(char *dir, char *name, char *what);
void create_yyyymmddhhmmss(char *name);

