/*
 * spamdrop.c, 2007.11.04, SJ
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


#ifdef HAVE_MYSQL
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

#ifdef HAVE_MYDB
   #include "mydb.h"
   int rc;
#endif


int main(int argc, char **argv){
   double spaminess=DEFAULT_SPAMICITY;
   struct stat st;
   struct timezone tz;
   struct timeval tv_spam_start, tv_spam_stop, tv1, tv2;
   struct passwd *pwd;
   struct session_data sdata;
   struct _state state;
   struct __config cfg;
   char buf[MAXBUFSIZE], qpath[SMALLBUFSIZE], *configfile=CONFIG_FILE, *username, *from=NULL;
   uid_t u;
   int i, n, fd, fd2, print_message=0, is_header=1, tot_len=0, put_subject_spam_prefix=0, sent_subject_spam_prefix=0, is_spam=0;
   int training_request=0, blackhole_request=0;
   FILE *f;

#ifndef HAVE_MYDB
   struct ue UE;
#endif

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

   /* shall we go in blackhole mode? */

#ifdef HAVE_BLACKHOLE
   if(strstr(argv[0], "blackhole")) blackhole_request = 1;
#endif


   cfg = read_config(configfile);

   /* maildrop exports the LOGNAME environment variable */

   username = getenv("LOGNAME");
   if(!username){
      u = getuid();
      pwd = getpwuid(u);
      username = pwd->pw_name;
   }

   snprintf(buf, MAXBUFSIZE-1, "%s/%s/%c/%s", cfg.chrootdir, USER_QUEUE_DIR, username[0], username);

#ifdef HAVE_MYDB
   if(strlen(cfg.mydbfile) < 4)
      snprintf(cfg.mydbfile, MAXVAL-1, "%s/%s/%c/%s/%s", cfg.chrootdir, USER_DATA_DIR, username[0], username, MYDB_FILE);
#endif

   if(stat(buf, &st) != 0){
      syslog(LOG_PRIORITY, "missing user directory: %s", buf);
      return EX_TEMPFAIL;
   }

   if(chdir(buf)){
      syslog(LOG_PRIORITY, "cannot chdir to %s", buf);
      return EX_TEMPFAIL;
   }


   sdata.num_of_rcpt_to = 1;
   sdata.uid = getuid();
   sdata.skip_id_check = 0;
   memset(sdata.rcptto[0], MAXBUFSIZE, 0);
   make_rnd_string(&(sdata.ttmpfile[0]));

   memset(trainbuf, 0, SMALLBUFSIZE);

   fd = open("/dev/stdin", O_RDONLY);
   if(fd == -1){
      syslog(LOG_PRIORITY, "cannot read stdin");
      return EX_TEMPFAIL;
   }

   fd2 = open(sdata.ttmpfile, O_CREAT|O_EXCL|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
   if(fd2 == -1){
      close(fd);
      syslog(LOG_PRIORITY, "cannot open: %s", sdata.ttmpfile);
      return EX_TEMPFAIL;
   }

   while((n = read(fd, buf, MAXBUFSIZE)) > 0){
      tot_len += n;
      write(fd2, buf, n);
   }

   close(fd);

   /* make sure we had a successful read */

   if(fsync(fd2)){
      syslog(LOG_PRIORITY, "failed writing data: %s", sdata.ttmpfile);
      return EX_TEMPFAIL;
   }

   close(fd2);

   if(cfg.verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: written %d bytes", sdata.ttmpfile, tot_len);

   /* check whether this is a training request with user+spam@... or user+ham@... */

   f = fopen(sdata.ttmpfile, "r");
   if(f){
      while(fgets(buf, MAXBUFSIZE-1, f)){
         if(strncmp(buf, "To:", 3) == 0 && (str_case_str(buf, "+ham@") || str_case_str(buf, "+spam@")) ){
            trim(buf);
            syslog(LOG_PRIORITY, "training request: %s", buf);
            training_request = 1;
            break;
         }

         if(buf[0] == '\r' || buf[0] == '\n') break;
      }
      fclose(f);
   }

   /* this is a training request */

   if(training_request == 1){
      from = getenv("FROM");

      if(!from) return 0;

      is_spam = 0;
      if(str_case_str(buf, "+spam@")) is_spam = 1;

   #ifdef HAVE_MYSQL
      mysql_init(&mysql);
      if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
         UE = get_user_from_email(mysql, from);
         sdata.uid = UE.uid;

         retraining(mysql, sdata, UE.name, is_spam, cfg);
         mysql_close(&mysql);
      }
   #endif
   #ifdef HAVE_SQLITE3
      UE = get_user_from_email(db, from);
      sdata.uid = UE.uid;

      retraining(db, sdata, UE.name, is_spam, cfg);
   #endif
   #ifdef HAVE_MYDB
      sdata.uid = 12345;

      rc = init_mydb(cfg.mydbfile, mhash);
      retraining(sdata, username, is_spam, cfg);
      close_mydb(mhash);
   #endif

      return 0;
   }



   gettimeofday(&tv_spam_start, &tz);

   if(tot_len <= cfg.max_message_size_to_filter){
      state = parse_message(sdata.ttmpfile, cfg);

   #ifdef HAVE_MYSQL
      mysql_init(&mysql);
      if(mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
         spaminess = bayes_file(mysql, sdata.ttmpfile, state, sdata, cfg);
         tum_train(sdata.ttmpfile, spaminess, cfg);
      }
      else
         syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_MYSQL_CONNECT);
   #endif
   #ifdef HAVE_SQLITE3
      rc = sqlite3_open(SQLITE3_DB_FILE, &db);
      if(rc){
         syslog(LOG_PRIORITY, "%s: %s", sdata.ttmpfile, ERR_SQLITE3_OPEN);
      }
      else {
         spaminess = bayes_file(db, sdata.ttmpfile, state, sdata, cfg);
         tum_train(sdata.ttmpfile, spaminess, cfg);
      }
   #endif
   #ifdef HAVE_MYDB
      rc = init_mydb(cfg.mydbfile, mhash);
      if(rc == 1){
         spaminess = bayes_file(sdata.ttmpfile, state, sdata, cfg);
         tum_train(sdata.ttmpfile, spaminess, cfg);
      }
   #endif

      /* if this a message to the blackhole */

      if(blackhole_request == 1){
         /* put IP address to blackhole directory */

         gettimeofday(&tv1, &tz);

         snprintf(buf, MAXBUFSIZE-1, "%s/%s", cfg.blackhole_path, state.ip);
         unlink(buf);

         fd = open(buf, O_RDWR|O_CREAT, S_IRUSR|S_IRGRP|S_IROTH);
         if(fd != -1){
            close(fd);
            gettimeofday(&tv2, &tz);
            syslog(LOG_PRIORITY, "putting %s to blackhole in %ld [us]", state.ip, tvdiff(tv2, tv1));
         }
         else syslog(LOG_PRIORITY, "failed to put %s to blackhole", state.ip);

         gettimeofday(&tv2, &tz);

         /* train with it if it is not recognised as spam */

         if(spaminess < cfg.spam_overall_limit){
            sdata.skip_id_check = 1;

            syslog(LOG_PRIORITY, "%s: retraining blackhole message", sdata.ttmpfile);
         #ifdef HAVE_MYSQL
            retraining(mysql, sdata, username, 1, cfg);
         #endif
         #ifdef HAVE_SQLITE3
            retraining(db, sdata, username, 1, cfg);
         #endif
         #ifdef HAVE_MYDB
            retraining(sdata, username, 1, cfg);
         #endif
         }
      }

   #ifdef HAVE_MYSQL
      mysql_close(&mysql);
   #endif
   #ifdef HAVE_SQLITE3
      sqlite3_close(db);
   #endif
   #ifdef HAVE_MYDB
      close_mydb(mhash);
   #endif

      free_and_print_list(state.first, 0);
   }


   /* rename file name according to its spamicity status, 2007.10.01, SJ */

   if(cfg.store_metadata == 1 && tot_len <= cfg.max_message_size_to_filter){
      if(spaminess >= cfg.spam_overall_limit)
         snprintf(qpath, SMALLBUFSIZE-1, "s.%s", sdata.ttmpfile);
      else
         snprintf(qpath, SMALLBUFSIZE-1, "h.%s", sdata.ttmpfile);

      link(sdata.ttmpfile, qpath);
      chmod(qpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
   }


   gettimeofday(&tv_spam_stop, &tz);

   syslog(LOG_PRIORITY, "%s: %.4f %d in %ld [ms]", sdata.ttmpfile, spaminess, tot_len, tvdiff(tv_spam_stop, tv_spam_start)/1000);

   if(print_message == 1){
      f = fopen(sdata.ttmpfile, "r");
      if(!f){
         syslog(LOG_PRIORITY, "cannot read: %s", sdata.ttmpfile);
         return EX_TEMPFAIL;
      }

      if(spaminess >= cfg.spam_overall_limit && spaminess < 1.01 && strlen(cfg.spam_subject_prefix) > 1) put_subject_spam_prefix = 1;

      while(fgets(buf, MAXBUFSIZE-1, f)){

         /* tag the Subject line if we have to, 2007.08.21, SJ */

         if(is_header == 1 && put_subject_spam_prefix == 1 && strncmp(buf, "Subject: ", 9) == 0 && !strstr(buf, cfg.spam_subject_prefix)){ 
            printf("Subject: ");
            printf("%s", cfg.spam_subject_prefix);
            printf("%s", &buf[9]);
            sent_subject_spam_prefix = 1;
         }

         if(is_header == 1 && (buf[0] == '\n' || buf[0] == '\r')){
            is_header = 0;

            printf("%s%s\r\n", cfg.clapf_header_field, sdata.ttmpfile);
            printf("%s%s%.4f\r\n", trainbuf, cfg.clapf_header_field, spaminess);
            printf("%s%ld ms\r\n", cfg.clapf_header_field, tvdiff(tv_spam_stop, tv_spam_start)/1000);
            if(spaminess > 0.9999) printf("%s%s\r\n", cfg.clapf_header_field, MSG_ABSOLUTELY_SPAM);
            if(spaminess >= cfg.spam_overall_limit && spaminess < 1.01){

               /* if we did not find a Subject line */

               if(sent_subject_spam_prefix == 0 && put_subject_spam_prefix == 1)
                  printf("Subject: %s\r\n", cfg.spam_subject_prefix);

               printf("%sYes\r\n", cfg.clapf_header_field);
            }
         }

         if(strncmp(buf, cfg.clapf_header_field, strlen(cfg.clapf_header_field)))
            printf("%s", buf);
      }

   }


   unlink(sdata.ttmpfile);

   snprintf(buf, MAXBUFSIZE-1, "%ld", sdata.uid);

   if(spaminess >= cfg.spam_overall_limit)
      log_ham_spam_per_email(sdata.ttmpfile, buf, 1);
   else
      log_ham_spam_per_email(sdata.ttmpfile, buf, 0);

   if(print_message == 0 && spaminess >= cfg.spam_overall_limit && spaminess < 1.01)
     return 1;

   return 0;
}
