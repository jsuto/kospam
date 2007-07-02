/*
 * passmail.c, 2006.09.06, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include "misc.h"
#include "cfg.h"
#include "user.h"
#include "datum.h"
#include "errmsg.h"
#include "messages.h"
#include "config.h"


int main(int argc, char **argv){
   int fd, fdspam, len, uid, gid, a=A_JUNK;
   char ttmpfile[RND_STR_LEN+1], spamfile[MAXBUFSIZE], buf[MAXBUFSIZE];
   struct passwd *pw;
   struct __config cfg;
   struct tm *t;
   struct userpref u;
   unsigned long ts;

   /* read the default or the given config file */

   if(argc < 2)
      cfg = read_config(CONFIG_FILE);
   else
      cfg = read_config(argv[1]);

   uid = getuid();
   gid = getgid();

   pw = getpwuid(uid);

   time(&ts);
   t = localtime(&ts);

   /* get user preference data from LDAP directory or mysql database */

#ifdef HAVE_USERDB

   #ifdef HAVE_USER_LDAP
      u = ldap_get_entry(cfg.ldap_host, cfg.ldap_base, cfg.ldap_user, cfg.ldap_pwd, cfg.ldap_use_tls, pw->pw_name);
   #endif

   #ifdef HAVE_USER_MYSQL
      u = mysql_get_entry(cfg.mysqlhost, cfg.mysqlport, cfg.mysqlsocket, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlusertable, pw->pw_name);
   #endif

#endif

   /* determine what action we have to take */

   if(strcmp(u.action, ACTION_DROP) == 0)
      a = A_DROP;
   else if(strcmp(u.action, ACTION_QUARANTINE) == 0)
      a = A_QUARANTINE;


   /* move spam to quarantine */

   if(a == A_QUARANTINE && strlen(cfg.spam_quarantine_dir) > 2){
      make_rnd_string(&ttmpfile[0]);

      snprintf(spamfile, MAXBUFSIZE-1, "%s/%s/%d%02d%02d%02d%02d%02d.%s", cfg.spam_quarantine_dir, pw->pw_name, t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, ttmpfile);

      //printf("uid: %d, gid: %d, home: %s, spamfile: %s\n", pw->pw_uid, gid, pw->pw_dir, spamfile);

      fd = open("/dev/stdin", O_RDONLY);
      if(fd != -1){
         fdspam = open(spamfile, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
         if(fdspam == -1){
            close(fd);
            snprintf(buf, MAXBUFSIZE-1, "%s: %s", ERR_CANNOT_OPEN, spamfile);
            __fatal(buf);
         }

         while((len = read(fd, buf, MAXBUFSIZE)) > 0)
            write(fdspam, buf, len);

         write(fdspam, "\n.\r\n", 4);

         close(fd);
         close(fdspam);

         chmod(spamfile, 0644);
      }

   }

   /* append spam to the junk folder */

   if(a == A_JUNK){

      if(chdir(pw->pw_dir)){
         snprintf(buf, MAXBUFSIZE-1, "%s: %s", ERR_CANNOT_CHDIR, pw->pw_dir);
         __fatal(buf);
      }

      fdspam = open(JUNK_FOLDER, O_CREAT|O_APPEND|O_RDWR, S_IRUSR|S_IWUSR);
      if(fdspam == -1){
         snprintf(buf, MAXBUFSIZE-1, "%s: %s", ERR_CANNOT_OPEN, JUNK_FOLDER);
         __fatal(buf);
      }

      flock(fdspam, LOCK_EX);

      fd = open("/dev/stdin", O_RDONLY);
      if(fd != -1){
         // From MaureenMichel@expertises-immo.com Thu Jan  5 04:34:59 2006
         snprintf(buf, MAXBUFSIZE-1, "From somespammer@dont.care %s %s %2d %02d:%02d:%02d %d\n", weekdays[t->tm_wday], months[t->tm_mon], t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, t->tm_year+1900);

         write(fdspam, buf, strlen(buf));

         while((len = read(fd, buf, MAXBUFSIZE)) > 0)
            write(fdspam, buf, len);

         close(fd);
         write(fdspam, "\n\n", 2);
      }

      flock(fdspam, LOCK_UN);
      close(fdspam);
   }


   /* silently discard message */

   if(a == A_DROP){
      fd = open("/dev/stdin", O_RDONLY);
      if(fd != -1){
         while(read(fd, buf, MAXBUFSIZE)){ }
         close(fd);
      }
   }


   return 0;
}
