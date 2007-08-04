/*
 * spamdrop.c, 2007.08.04, SJ
 *
 * check if a single RFC-822 formatted messages is spam or not
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#include <sysexits.h>
#include "misc.h"
#include "bayes.h"
#include "errmsg.h"
#include "messages.h"
#include "sql.h"
#include "config.h"


extern char *optarg;
extern int optind;


#ifdef HAVE_MYSQL_TOKEN_DATABASE
   #include <mysql.h>
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;
   int rc;
#endif


int main(int argc, char **argv){
   double spaminess=DEFAULT_SPAMICITY;
   struct stat st;
   struct timezone tz;
   struct timeval tv_spam_start, tv_spam_stop;
   struct session_data sdata;
   struct __config cfg;
   char buf[MAXBUFSIZE], *configfile=CONFIG_FILE;
   int i, n, x, fd, fd2, print_message=0, is_header=1, tot_len=0;
   FILE *f;

   while((i = getopt(argc, argv, "c:p")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'p' :
                    print_message = 1;
                    break;

         default  : 
                    break;
       }
   }

   if(argc < 2){
      fprintf(stderr, "usage: %s [-c <config file>] [-p] < <RFC-822 formatted message>\n", argv[0]);
      return 0;
   }


   (void) openlog("spamdrop", LOG_PID, LOG_MAIL);

   cfg = read_config(configfile);

   if(stat(PER_USER_DIR, &st) != 0){
      mkdir(PER_USER_DIR, 0700);
   }
   chdir(PER_USER_DIR);

   sdata.num_of_rcpt_to = 1;
   sdata.uid = getuid();
   memset(sdata.rcptto[0], MAXBUFSIZE, 0);
   make_rnd_string(&(sdata.ttmpfile[0]));

   memset(trainbuf, 0, SMALLBUFSIZE);

   fd = open("/dev/stdin", O_RDONLY);
   if(fd == -1) return EX_TEMPFAIL;

   fd2 = open(sdata.ttmpfile, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
   if(fd2 == -1){
      close(fd);
      return EX_TEMPFAIL;
   }

   while((n = read(fd, buf, MAXBUFSIZE)) > 0){
      tot_len += n;
      write(fd2, buf, n);
   }

   close(fd);
   close(fd2);

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: written temporary file", sdata.ttmpfile);

   gettimeofday(&tv_spam_start, &tz);

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   mysql_init(&mysql);
   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      spaminess = bayes_file(mysql, sdata.ttmpfile, sdata, cfg);

      x = update_training_metadata(mysql, sdata.ttmpfile, sdata.uid, cfg);
      mysql_close(&mysql);

      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: update metadata: %d", sdata.ttmpfile, x);
   }
   else
      syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_MYSQL_CONNECT);
#endif
#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc){
      syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_SQLITE3_OPEN);
   }
   else {
      spaminess = bayes_file(db, sdata.ttmpfile, sdata, cfg);
   }
#endif

   gettimeofday(&tv_spam_stop, &tz);

   syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, spaminess, tot_len, tvdiff(tv_spam_stop, tv_spam_start)/1000);

   if(print_message == 1){
      f = fopen(sdata.ttmpfile, "r");
      if(!f) return EX_TEMPFAIL;

      while(fgets(buf, MAXBUFSIZE-1, f)){
         if(is_header == 1 && (buf[0] == '\n' || buf[0] == '\r')){
            is_header = 0;
            printf("%s%s\r\n", cfg.clapf_header_field, sdata.ttmpfile);
            printf("%s%s%.4f\r\n", trainbuf, cfg.clapf_header_field, spaminess);
            printf("%s%ld ms\r\n", cfg.clapf_header_field, tvdiff(tv_spam_stop, tv_spam_start)/1000);
            if(spaminess > 0.9999) printf("%s%s\r\n", cfg.clapf_header_field, MSG_ABSOLUTELY_SPAM);
            if(spaminess >= cfg.spam_overall_limit && spaminess < 1.01) printf("%sYes\r\n", cfg.clapf_header_field);
         }
         if(strncmp(buf, cfg.clapf_header_field, strlen(cfg.clapf_header_field)))
            printf("%s", buf);
      }

   }


   unlink(sdata.ttmpfile);

   if(print_message == 0 && spaminess >= cfg.spam_overall_limit && spaminess < 1.01)
     return 1;

   return 0;
}
