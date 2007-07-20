/*
 * spamdrop.c, 2007.07.06, SJ
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

void print_header(char *s, char *h){
   char *p, puf[MAXBUFSIZE];

   p = s;

   do {
      p = split(p, '\n', puf, MAXBUFSIZE-1);
      if(strncmp(puf, h, strlen(h))) printf("%s\n", puf);
   } while(p);
}

int main(int argc, char **argv){
   double spaminess=DEFAULT_SPAMICITY;
   struct timezone tz;
   struct timeval tv_spam_start, tv_spam_stop;
   struct session_data sdata;
   struct __config cfg;
   char buf[MAXBUFSIZE], *configfile=CONFIG_FILE, *p=NULL, *q=NULL;
   unsigned long uid;
   int i, n, x, fd, totlen=0, print_message=0;

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


   uid = getuid();

   cfg = read_config(configfile);

   sdata.num_of_rcpt_to = 1;
   memset(sdata.rcptto[0], MAXBUFSIZE, 0);
   make_rnd_string(&(sdata.ttmpfile[0]));


   /* select uid or email from user table */

   snprintf(buf, MAXBUFSIZE-1, "SELECT email FROM %s WHERE uid=%ld", SQL_USER_TABLE, uid);

#ifdef HAVE_MYSQL_TOKEN_DATABASE
   mysql_init(&mysql);

   if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){

      if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
         while((res = mysql_store_result(&mysql))){
            row = mysql_fetch_row(res);
            if(row){
               snprintf(sdata.rcptto[0], SMALLBUFSIZE-1, "<%s>", row[0]);
            }
         }
         mysql_free_result(res);
      }
   }
   else 
      syslog(LOG_PRIORITY, "%s", ERR_MYSQL_CONNECT);

#endif
#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc){
      syslog(LOG_PRIORITY, "%s", ERR_SQLITE3_OPEN);
   }
   else {
      if(sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail) == SQLITE_OK){
         while(sqlite3_step(pStmt) == SQLITE_ROW){
            snprintf(sdata.rcptto[0], SMALLBUFSIZE-1, "<%s>", sqlite3_column_text(pStmt, 0));
         }
      } 
      sqlite3_finalize(pStmt);
   }

#endif


   sdata.uid = uid;

   syslog(LOG_PRIORITY, "%s: uid: %ld", sdata.ttmpfile, uid);

   fd = open("/dev/stdin", O_RDONLY);
   if(fd == -1) return EX_TEMPFAIL;

   p = malloc(MAXBUFSIZE);
   if(!p) return EX_TEMPFAIL;

   q = p;

   while((n = read(fd, buf, MAXBUFSIZE)) > 0){
      memcpy(p+totlen, buf, n);
      totlen += n;

      q = realloc(q, totlen+n);
      if(!q){
         if(p) free(p);
         close(fd);
         return EX_TEMPFAIL;
      }
   }
   close(fd);

   fd = open(sdata.ttmpfile, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
   if(fd != 1){
      write(fd, p, totlen);
      close(fd);

      gettimeofday(&tv_spam_start, &tz);

   #ifdef HAVE_MYSQL_TOKEN_DATABASE
      spaminess = bayes_file(mysql, sdata.ttmpfile, sdata, cfg);

      x = update_training_metadata(mysql, sdata.ttmpfile, sdata.rcptto, sdata.num_of_rcpt_to, cfg);
      if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "update metadata result: %d", x);

   #endif
   #ifdef HAVE_SQLITE3
      spaminess = bayes_file(db, sdata.ttmpfile, sdata, cfg);
   #endif

      gettimeofday(&tv_spam_stop, &tz);

      unlink(sdata.ttmpfile);
   }

   if(print_message == 1){

      if((q = strstr(p, "\n\n"))){
         *q = '\0';
      }
      else if((q = strstr(p, "\r\n\r\n"))){
         *q = '\0';
      }

      if(!q){
         if(p) free(p);
         return EX_TEMPFAIL;
      }

      print_header(p, cfg.clapf_header_field);

      printf("%s%s\r\n", cfg.clapf_header_field, sdata.ttmpfile);
      printf("%s%.4f\r\n", cfg.clapf_header_field, spaminess);
      printf("%s%ld ms\r\n", cfg.clapf_header_field, tvdiff(tv_spam_stop, tv_spam_start)/1000);
      if(spaminess >= cfg.spam_overall_limit && spaminess < 1.01) printf("%sYes\r\n", cfg.clapf_header_field);
      q++;
      printf("%s", q);
      
   }

   if(p) free(p);

   if(print_message == 0 && spaminess >= cfg.spam_overall_limit && spaminess < 1.01)
     return 1;

   return 0;
}
