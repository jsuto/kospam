/*
 * spam.c, 2009.04.29, SJ
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include "defs.h"
#include "misc.h"
#include "av.h"
#include "templates.h"
#include "list.h"
#include "session.h"
#include "config.h"


/*
 * train this message
 */

void do_training(struct session_data *sdata, char *email, char *acceptbuf, struct __config *cfg){
#ifndef HAVE_MYDB
   int is_spam = 0;
   int train_mode;
   char qpath[SMALLBUFSIZE], ID[RND_STR_LEN+1];
   struct _state sstate2;


   snprintf(acceptbuf, MAXBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata->ttmpfile, email);
   if(strcasestr(sdata->rcptto[0], "+spam@") || strncmp(email, "spam@", 5) == 0) is_spam = 1;

   train_mode = extract_id_from_message(sdata->ttmpfile, cfg->clapf_header_field, ID);

   syslog(LOG_PRIORITY, "%s: training request for %s by uid: %ld", sdata->ttmpfile, ID, sdata->uid);

   if(strlen(ID) < 5){
      syslog(LOG_PRIORITY, "%s: not found a valid message id (%s)", sdata->ttmpfile, ID);
      return;
   }

   if(is_spam == 1)
      snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, sdata->name[0], sdata->name, ID);
   else
      snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, sdata->name[0], sdata->name, ID);

   sstate2 = parse_message(qpath, sdata, cfg);

   train_message(sdata, &sstate2, MAX_ITERATIVE_TRAIN_LOOPS, is_spam, train_mode, cfg);

   free_list(sstate2.urls);
   clearhash(sstate2.token_hash, 0);
#endif

}


/*
 * save message to queue (local filesystem or mysql database)
 */

#ifdef HAVE_STORE
void save_email_to_queue(struct session_data *sdata, float spaminess, struct __config *cfg){
   struct stat st;
   struct timezone tz;
   struct timeval tv1, tv2;

   if(cfg->store_metadata == 0 || strlen(sdata->name) <= 1) return;

   if(cfg->store_only_spam == 1 && spaminess < cfg->spam_overall_limit) return;

   gettimeofday(&tv1, &tz);

#ifdef STORE_FS
   char qpath[SMALLBUFSIZE];

   snprintf(qpath, SMALLBUFSIZE-1, "%s/%c", USER_QUEUE_DIR, sdata->name[0]);
   if(stat(qpath, &st)) mkdir(qpath, QUEUE_DIR_PERMISSION);

   snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s", USER_QUEUE_DIR, sdata->name[0], sdata->name);
   if(stat(qpath, &st)){
      mkdir(qpath, QUEUE_DIR_PERMISSION);

      /*
       * the web server must have write permissions on the user's queue directory.
       * you have to either extend these rights the to world, ie. 777 or
       * change group-id of clapf to the web server, ie. usermod -g www-data clapf.
       * 2008.10.27, SJ
       */

       chmod(qpath, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP);
    }

    if(spaminess >= cfg->spam_overall_limit)
       snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, sdata->name[0], sdata->name, sdata->ttmpfile);
    else
       snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, sdata->name[0], sdata->name, sdata->ttmpfile);

    link(sdata->ttmpfile, qpath);
    if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: try to link to %s", sdata->ttmpfile, qpath);

    if(stat(qpath, &st) == 0){
       if(S_ISREG(st.st_mode) == 1) chmod(qpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    }
#endif

#ifdef STORE_NFS
   char qpath[SMALLBUFSIZE];
   char buf[MAXBUFSIZE];
   int n, fd, fd2;

   snprintf(qpath, SMALLBUFSIZE-1, "%s/%c", USER_QUEUE_DIR, sdata->name[0]);
   if(stat(qpath, &st)) mkdir(qpath, QUEUE_DIR_PERMISSION);

   snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s", USER_QUEUE_DIR, sdata->name[0], sdata->name);
   if(stat(qpath, &st)){
      mkdir(qpath, QUEUE_DIR_PERMISSION);

      /*
       * the web server must have write permissions on the user's queue directory.
       * you have to either extend these rights the to world, ie. 777 or
       * change group-id of clapf to the web server, ie. usermod -g www-data clapf.
       * 2008.10.27, SJ
       */

       chmod(qpath, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP);
   }

    if(spaminess >= cfg->spam_overall_limit)
       snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/s.%s", USER_QUEUE_DIR, sdata->name[0], sdata->name, sdata->ttmpfile);
    else
       snprintf(qpath, SMALLBUFSIZE-1, "%s/%c/%s/h.%s", USER_QUEUE_DIR, sdata->name[0], sdata->name, sdata->ttmpfile);

   /* copy here */

   fd = open(sdata->ttmpfile, O_RDONLY);
   if(fd == -1){
      syslog(LOG_PRIORITY, "%s: cannot open to store", sdata->ttmpfile);
      return;
   }

   fd2 = open(qpath, O_CREAT|O_EXCL|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
   if(fd2 == -1){
      close(fd);
      syslog(LOG_PRIORITY, "%s: cannot open for save: %s", sdata->ttmpfile, qpath);
      return;
   }

   /* copy tmpfile to NFS */

   while((n = read(fd, buf, MAXBUFSIZE)) > 0){
      write(fd2, buf, n);
   }

   close(fd);
   close(fd2);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: try to copy to %s", sdata->ttmpfile, qpath);

   if(stat(qpath, &st) == 0){
      if(S_ISREG(st.st_mode) == 1) chmod(qpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
   }

#endif

#ifdef STORE_MYSQL
   #include <mysql.h>

   char *data=NULL;
   char buf[MAXBUFSIZE], *map=NULL;
   unsigned long now=0;
   int fd, is_spam=0;
   time_t clock;

   time(&clock);
   now = clock;

   if(spaminess >= cfg->spam_overall_limit) is_spam = 1;

   /* reading message file into memory */

   if(stat(sdata->ttmpfile, &st)){
      syslog(LOG_PRIORITY, "%s: cannot stat before putting to queue", sdata->ttmpfile);
      return;
   }

   fd = open(sdata->ttmpfile, O_RDONLY);
   if(fd == -1) return;

   map = mmap(map, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
   close(fd);
   if(map == NULL) return;

   /* then put it into database */

   data = malloc(2 * st.st_size + strlen(buf) + 1 + 1 + 1);
   if(!data){
      goto ENDE;
   }

   snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (id, uid, is_spam, ts, data) VALUES('%s', %ld, %d, %ld, \"", SQL_QUEUE_TABLE, sdata->ttmpfile, sdata->uid, is_spam, now);
   snprintf(data, 2 * st.st_size + strlen(buf) + 1, "%s", buf);
   mysql_real_escape_string(&(sdata->mysql), data+strlen(buf), map, st.st_size);
   strncat(data, "\")", 2 * st.st_size + strlen(buf) + 1 + 1);
   mysql_real_query(&(sdata->mysql), data, strlen(data));

   free(data);

ENDE:

   munmap(map, st.st_size);

#endif

    gettimeofday(&tv2, &tz);
    if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: saved to queue: %ld [ms]", sdata->ttmpfile, tvdiff(tv2, tv1)/1000);
}
#endif

