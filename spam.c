/*
 * spam.c, 2009.01.20, SJ
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
 * calculate spaminess
 */

void save_email_to_queue(struct session_data *sdata, float spaminess, struct __config *cfg){
   char qpath[SMALLBUFSIZE];
   struct stat st;

   if(cfg->store_metadata == 0 || strlen(sdata->name) <= 1) return;

   if(cfg->store_only_spam == 1 && spaminess < cfg->spam_overall_limit) return;

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

}


