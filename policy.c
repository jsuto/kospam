/*
 * policy.c, 2008.11.26, SJ
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
            if(row[3] != NULL) snprintf(my_cfg->spam_subject_prefix, MAXVAL-1, "%s ", row[3]);
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


#ifdef USERS_IN_LDAP
int get_policy(LDAP *ld, char *base, struct __config *cfg, struct __config *my_cfg, unsigned int policy_group, int num_of_rcpt_to){
   int rc;
   char filter[SMALLBUFSIZE], *attrs[] = { NULL }, **vals;
   LDAPMessage *res, *e;

#ifndef HAVE_LMTP
   if(num_of_rcpt_to != 1) return 0;
#endif

   if(ld == NULL) return 0;

   snprintf(filter, SMALLBUFSIZE-1, "(policyGroup=%d)", policy_group);

   rc = ldap_search_s(ld, base, LDAP_SCOPE, filter, attrs, 0, &res);
   if(rc) return 0;

   e = ldap_first_entry(ld, res);

   if(e){
      vals = ldap_get_values(ld, e, "deliverinfectedemail");
      if(ldap_count_values(vals) > 0) my_cfg->deliver_infected_email = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "silentlydiscardinfectedemail");
      if(ldap_count_values(vals) > 0) my_cfg->silently_discard_infected_email = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "useantispam");
      if(ldap_count_values(vals) > 0) my_cfg->use_antispam = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "spamsubjectprefix");
      if(ldap_count_values(vals) > 0) snprintf(my_cfg->spam_subject_prefix, MAXVAL-1, "%s", vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "enableautowhitelist");
      if(ldap_count_values(vals) > 0) my_cfg->enable_auto_white_list = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "maxmessagesizetofilter");
      if(ldap_count_values(vals) > 0) my_cfg->max_message_size_to_filter = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "rbldomain");
      if(ldap_count_values(vals) > 0) snprintf(my_cfg->rbl_domain, MAXVAL-1, "%s", vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "surbldomain");
      if(ldap_count_values(vals) > 0) snprintf(my_cfg->surbl_domain, MAXVAL-1, "%s", vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "spamoveralllimit");
      if(ldap_count_values(vals) > 0) my_cfg->spam_overall_limit = atof(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "spaminessoblivionlimit");
      if(ldap_count_values(vals) > 0) my_cfg->spaminess_oblivion_limit = atof(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "replacejunkcharacters");
      if(ldap_count_values(vals) > 0) my_cfg->replace_junk_characters = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "invalidjunklimit");
      if(ldap_count_values(vals) > 0) my_cfg->invalid_junk_limit = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "invalidjunkline");
      if(ldap_count_values(vals) > 0) my_cfg->invalid_junk_line = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "penalizeimages");
      if(ldap_count_values(vals) > 0) my_cfg->penalize_images = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "penalizeembedimages");
      if(ldap_count_values(vals) > 0) my_cfg->penalize_embed_images = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "penalizeoctetstream");
      if(ldap_count_values(vals) > 0) my_cfg->penalize_octet_stream = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "trainingmode");
      if(ldap_count_values(vals) > 0) my_cfg->training_mode = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(ld, e, "initial1000learning");
      if(ldap_count_values(vals) > 0) my_cfg->initial_1000_learning = atoi(vals[0]);
      ldap_value_free(vals);

   }


   ldap_msgfree(res);

   return 1;
}

#endif
