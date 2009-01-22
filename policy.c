/*
 * policy.c, 2009.01.22, SJ
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
int get_policy(struct session_data *sdata, struct __config *cfg, struct __config *my_cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char buf[SMALLBUFSIZE];

#ifndef HAVE_LMTP
   if(sdata->num_of_rcpt_to != 1) return 0;
#endif

   snprintf(buf, SMALLBUFSIZE-1, "SELECT deliver_infected_email, silently_discard_infected_email, use_antispam, spam_subject_prefix, enable_auto_white_list, max_message_size_to_filter, rbl_domain, surbl_domain, spam_overall_limit, spaminess_oblivion_limit, replace_junk_characters, invalid_junk_limit, invalid_junk_line, penalize_images, penalize_embed_images, penalize_octet_stream, training_mode, initial_1000_learning FROM %s WHERE policy_group=%d", SQL_POLICY_TABLE, sdata->policy_group);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "policy sql: %s", buf);

   if(mysql_real_query(&(sdata->mysql), buf, strlen(buf)) == 0){
      res = mysql_store_result(&(sdata->mysql));
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
int get_policy(struct session_data *sdata, struct __config *cfg, struct __config *my_cfg){
   int rc;
   char filter[SMALLBUFSIZE], *attrs[] = { NULL }, **vals;
   LDAPMessage *res, *e;

#ifndef HAVE_LMTP
   if(sdata->num_of_rcpt_to != 1) return 0;
#endif

   if(sdata->ldap == NULL) return 0;

   snprintf(filter, SMALLBUFSIZE-1, "(policyGroup=%d)", sdata->policy_group);

   rc = ldap_search_s(sdata->ldap, cfg->ldap_base, LDAP_SCOPE, filter, attrs, 0, &res);
   if(rc) return 0;

   e = ldap_first_entry(sdata->ldap, res);

   if(e){
      vals = ldap_get_values(sdata->ldap, e, "deliverinfectedemail");
      if(ldap_count_values(vals) > 0) my_cfg->deliver_infected_email = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "silentlydiscardinfectedemail");
      if(ldap_count_values(vals) > 0) my_cfg->silently_discard_infected_email = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "useantispam");
      if(ldap_count_values(vals) > 0) my_cfg->use_antispam = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "spamsubjectprefix");
      if(ldap_count_values(vals) > 0) snprintf(my_cfg->spam_subject_prefix, MAXVAL-1, "%s", vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "enableautowhitelist");
      if(ldap_count_values(vals) > 0) my_cfg->enable_auto_white_list = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "maxmessagesizetofilter");
      if(ldap_count_values(vals) > 0) my_cfg->max_message_size_to_filter = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "rbldomain");
      if(ldap_count_values(vals) > 0) snprintf(my_cfg->rbl_domain, MAXVAL-1, "%s", vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "surbldomain");
      if(ldap_count_values(vals) > 0) snprintf(my_cfg->surbl_domain, MAXVAL-1, "%s", vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "spamoveralllimit");
      if(ldap_count_values(vals) > 0) my_cfg->spam_overall_limit = atof(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "spaminessoblivionlimit");
      if(ldap_count_values(vals) > 0) my_cfg->spaminess_oblivion_limit = atof(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "replacejunkcharacters");
      if(ldap_count_values(vals) > 0) my_cfg->replace_junk_characters = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "invalidjunklimit");
      if(ldap_count_values(vals) > 0) my_cfg->invalid_junk_limit = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "invalidjunkline");
      if(ldap_count_values(vals) > 0) my_cfg->invalid_junk_line = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "penalizeimages");
      if(ldap_count_values(vals) > 0) my_cfg->penalize_images = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "penalizeembedimages");
      if(ldap_count_values(vals) > 0) my_cfg->penalize_embed_images = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "penalizeoctetstream");
      if(ldap_count_values(vals) > 0) my_cfg->penalize_octet_stream = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "trainingmode");
      if(ldap_count_values(vals) > 0) my_cfg->training_mode = atoi(vals[0]);
      ldap_value_free(vals);

      vals = ldap_get_values(sdata->ldap, e, "initial1000learning");
      if(ldap_count_values(vals) > 0) my_cfg->initial_1000_learning = atoi(vals[0]);
      ldap_value_free(vals);

   }


   ldap_msgfree(res);

   return 1;
}

#endif
