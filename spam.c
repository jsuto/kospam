/*
 * spam.c, 2009.09.26, SJ
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
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
 * create path from numeric uid or username
 */

void get_queue_path(struct session_data *sdata, char **path){
   struct stat st;

   memset(*path, 0, SMALLBUFSIZE);

#ifdef HAVE_UID_SPLITTING
   unsigned int h, i, plus1, plus1b;

   if(sdata->uid <= 0) return;

   h = sdata->uid;

   i = h % 10000;
   if(i > 0) plus1 = 1;
   else plus1 = 0;

   i = h % 100;
   if(i > 0) plus1b = 1;
   else plus1b = 0;

   snprintf(*path, SMALLBUFSIZE-1, "%s/%d/%d/%ld", USER_QUEUE_DIR, 10000 * ((h / 10000) + plus1), 100 * ((h / 100) + plus1b), sdata->uid);

   /* create target directory if it doesn't exist */

   if(stat(*path, &st) != 0){
      snprintf(*path, SMALLBUFSIZE-1, "%s/%d", USER_QUEUE_DIR, 10000 * ((h / 10000) + plus1));
      mkdir(*path, 0755);

      snprintf(*path, SMALLBUFSIZE-1, "%s/%d/%d", USER_QUEUE_DIR, 10000 * ((h / 10000) + plus1), 100 * ((h / 100) + plus1b));
      mkdir(*path, 0755);

      snprintf(*path, SMALLBUFSIZE-1, "%s/%d/%d/%ld", USER_QUEUE_DIR, 10000 * ((h / 10000) + plus1), 100 * ((h / 100) + plus1b), sdata->uid);
      mkdir(*path, 0755);

      chmod(*path, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP);
   }
#else
   snprintf(*path, SMALLBUFSIZE-1, "%s/%s/%c/%s", USER_QUEUE_DIR, sdata->domain, sdata->name[0], sdata->name);

   /* create target directory if it doesn't exist */

   if(stat(*path, &st) != 0){
      snprintf(*path, SMALLBUFSIZE-1, "%s/%s", USER_QUEUE_DIR, sdata->domain);
      mkdir(*path, 0755);

      snprintf(*path, SMALLBUFSIZE-1, "%s/%s/%c", USER_QUEUE_DIR, sdata->domain, sdata->name[0]);
      mkdir(*path, 0755);

      snprintf(*path, SMALLBUFSIZE-1, "%s/%s/%c/%s", USER_QUEUE_DIR, sdata->domain, sdata->name[0], sdata->name);
      mkdir(*path, 0755);

      chmod(*path, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP);
   }

#endif
}


/*
 * train this message
 */

void do_training(struct session_data *sdata, struct _state *state, char *email, char *acceptbuf, struct __config *cfg){
#ifndef HAVE_MYDB
   int is_spam = 0;
   char qpath[SMALLBUFSIZE], *p, path[SMALLBUFSIZE];

   snprintf(acceptbuf, SMALLBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata->ttmpfile, email);
   if(strcasestr(sdata->rcptto[0], "+spam@") || strncmp(email, "spam@", 5) == 0) is_spam = 1;

   p = &path[0];
   get_queue_path(sdata, &p);

#ifdef HAVE_STORE
   int train_mode;
   char ID[RND_STR_LEN+1];
   struct _state sstate2;

   train_mode = extract_id_from_message(sdata->ttmpfile, cfg->clapf_header_field, ID);

   syslog(LOG_PRIORITY, "%s: training request for %s by uid: %ld", sdata->ttmpfile, ID, sdata->uid);

   if(strlen(ID) < 5){
      syslog(LOG_PRIORITY, "%s: not found a valid message id (%s)", sdata->ttmpfile, ID);
      return;
   }


   if(is_spam == 1){
      snprintf(qpath, SMALLBUFSIZE-1, "%s/h.%s", path, ID);
   }
   else {
      snprintf(qpath, SMALLBUFSIZE-1, "%s/s.%s", path, ID);
   }

   sstate2 = parse_message(qpath, sdata, cfg);

   train_message(sdata, &sstate2, MAX_ITERATIVE_TRAIN_LOOPS, is_spam, train_mode, cfg);

   free_list(sstate2.urls);
   clearhash(sstate2.token_hash, 0);
#else
   struct stat st;

   if(is_spam == 1){
      snprintf(qpath, SMALLBUFSIZE-1, "%s/h.%s", path, sdata->clapf_id);
   } else {
      snprintf(qpath, SMALLBUFSIZE-1, "%s/s.%s", path, sdata->clapf_id);
   }

   syslog(LOG_PRIORITY, "%s: checking %s for training", sdata->ttmpfile, qpath);

   if(stat(qpath, &st) == 0 && S_ISREG(st.st_mode) == 1){
      train_message(sdata, state, MAX_ITERATIVE_TRAIN_LOOPS, is_spam, state->train_mode, cfg);
   }
   else {
      syslog(LOG_PRIORITY, "%s: invalid signature: %s", sdata->ttmpfile, qpath);
   }
#endif

#endif

}


/*
 * save message to queue (local or networked filesystem)
 */

void save_email_to_queue(struct session_data *sdata, float spaminess, struct __config *cfg){
   int touch;
   char *p, path[SMALLBUFSIZE], qpath[SMALLBUFSIZE];

   if(strlen(sdata->name) <= 1) return;

#ifndef HAVE_STORE
   if(cfg->store_metadata == 0) return;
   if(cfg->store_only_spam == 1 && spaminess < cfg->spam_overall_limit) return;
#endif

   p = &path[0];
   get_queue_path(sdata, &p);

   if(spaminess >= cfg->spam_overall_limit)
      snprintf(qpath, SMALLBUFSIZE-1, "%s/s.%s", path, sdata->ttmpfile);
   else
      snprintf(qpath, SMALLBUFSIZE-1, "%s/h.%s", path, sdata->ttmpfile);

#ifdef HAVE_STORE
   if(cfg->store_metadata == 0 || (cfg->store_only_spam == 1 && spaminess < cfg->spam_overall_limit) ) goto TOUCH;
#endif


#ifdef STORE_FS
   struct stat st;

   link(sdata->ttmpfile, qpath);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: saving to queue: %s", sdata->ttmpfile, qpath);

   if(stat(qpath, &st) == 0){
      if(S_ISREG(st.st_mode) == 1) chmod(qpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
   }

   return;
#endif

#ifdef STORE_NFS
   char buf[MAXBUFSIZE];
   int n, fd, fd2;
   struct stat st;

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

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: saving to queue: %s", sdata->ttmpfile, qpath);

   if(stat(qpath, &st) == 0){
      if(S_ISREG(st.st_mode) == 1) chmod(qpath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
   }

   return;
#endif

#ifdef HAVE_STORE
TOUCH:
#endif

   /* emulating 'touch' */

   touch = open(qpath, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
   if(touch != -1) close(touch);

}

