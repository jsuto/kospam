/*
 * policy.c, 2008.11.25, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <clapf.h>


#ifdef USERS_IN_MYSQL
   #include <mysql.h>
#endif


/*
 * get user policy
 */

#ifdef USERS_IN_MYSQL
int get_policy(MYSQL mysql, struct __config *cfg, struct __config *my_cfg, unsigned int policy_group, int num_of_rcpt_to){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char buf[SMALLBUFSIZE];

#ifndef HAVE_LMTP
   if(num_of_rcpt_to != 1) return 0;
#endif

   snprintf(buf, SMALLBUFSIZE-1, "SELECT deliver_infected_email, silently_discard_infected_email, use_antispam, spam_subject_prefix, enable_auto_white_list, max_message_size_to_filter, rbl_domain, surbl_domain, spam_overall_limit, spaminess_oblivion_limit, replace_junk_characters, invalid_junk_limit, invalid_junk_line, penalize_images, penalize_embed_images, penalize_octet_stream, training_mode, initial_1000_learning FROM %s WHERE policy_group=%d", SQL_POLICY_TABLE, policy_group);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "policy sql: %s", buf);

   if(mysql_real_query(&mysql, buf, strlen(buf)) == 0){
      res = mysql_store_result(&mysql);
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row){
            my_cfg->deliver_infected_email = atoi(row[0]);
            my_cfg->silently_discard_infected_email = atoi(row[1]);
            my_cfg->use_antispam = atoi(row[2]);
            if(row[3] != NULL) snprintf(my_cfg->spam_subject_prefix, MAXVAL-1, "%s", row[3]);
            my_cfg->enable_auto_white_list = atoi(row[4]);
            my_cfg->max_message_size_to_filter = atoi(row[5]);
            if(row[6] != NULL) snprintf(my_cfg->rbl_domain, MAXVAL-1, "%s", row[6]);
            if(row[7] != NULL) snprintf(my_cfg->surbl_domain, MAXVAL-1, "%s", row[7]);
            my_cfg->spam_overall_limit = atof(row[8]);
            my_cfg->spaminess_oblivion_limit = atof(row[9]);
            my_cfg->replace_junk_characters = atoi(row[10]);
            my_cfg->invalid_junk_limit = atoi(row[11]);
            my_cfg->invalid_junk_line = atoi(row[12]);
            my_cfg->penalize_images = atoi(row[13]);
            my_cfg->penalize_embed_images = atoi(row[14]);
            my_cfg->penalize_octet_stream = atoi(row[15]);
            my_cfg->training_mode = atoi(row[16]);
            my_cfg->initial_1000_learning = atoi(row[17]);
         }
         mysql_free_result(res);
      }
   }

   return 1;
}

#endif


