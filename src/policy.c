/*
 * policy.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <clapf.h>


int get_policy(struct session_data *sdata, struct __data *data, struct __config *cfg, struct __config *my_cfg){
   int rc=0;
   float f;
   char f1[MAXVAL], f2[MAXVAL];

   /*
    * in case of smtp mode don't query unless we have a single recipient
    */

   if(cfg->server_mode == SMTP_MODE && sdata->num_of_rcpt_to != 1) return rc;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: policy sql (%d): %s", sdata->ttmpfile, sdata->policy_group, SQL_PREPARED_STMT_QUERY_POLICY);


   if(prepare_sql_statement(sdata, &(data->stmt_get_policy), SQL_PREPARED_STMT_QUERY_POLICY) == ERR) return rc;

   p_bind_init(data);

   data->sql[data->pos] = (char *)&(sdata->policy_group); data->type[data->pos] = TYPE_LONG; data->pos++;

   if(p_exec_query(sdata, data->stmt_get_policy, data) == ERR) goto ENDE;

   p_bind_init(data);

   data->sql[data->pos] = (char *)&(my_cfg->deliver_infected_email); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->silently_discard_infected_email); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->use_antispam); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = my_cfg->spam_subject_prefix; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = MAXVAL-1; data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->max_message_size_to_filter); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = my_cfg->surbl_domain; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = MAXVAL-1; data->pos++;
   data->sql[data->pos] = &f1[0]; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = MAXVAL-1; data->pos++;
   data->sql[data->pos] = &f2[0]; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = MAXVAL-1; data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->replace_junk_characters); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->penalize_images); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->penalize_embed_images); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->penalize_octet_stream); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->training_mode); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->store_emails); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->store_only_spam); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->message_from_a_zombie); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = my_cfg->smtp_addr; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = MAXVAL-1; data->pos++;
   data->sql[data->pos] = (char *)&(my_cfg->smtp_port); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;


   p_store_results(sdata, data->stmt_get_policy, data);

   if(p_fetch_results(data->stmt_get_policy) == OK){
      f = atof(f1); if(f > 0.1) my_cfg->spam_overall_limit = f;
      f = atof(f2); if(f > 0.1) my_cfg->spaminess_oblivion_limit = f;

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: policy settings: %d/%d/%d/%s/%d/%s/%.4f/%.4f/%d/%d/%d/%d/%d/%d/%d/%d/%s/%d",
                                              sdata->ttmpfile, my_cfg->deliver_infected_email, my_cfg->silently_discard_infected_email, my_cfg->use_antispam,
                                              my_cfg->spam_subject_prefix, my_cfg->max_message_size_to_filter, my_cfg->surbl_domain, my_cfg->spam_overall_limit,
                                              my_cfg->spaminess_oblivion_limit, my_cfg->replace_junk_characters, my_cfg->penalize_images, my_cfg->penalize_embed_images,
                                              my_cfg->penalize_octet_stream, my_cfg->training_mode, my_cfg->store_emails, my_cfg->store_only_spam, my_cfg->message_from_a_zombie,
                                              my_cfg->smtp_addr, my_cfg->smtp_port);
   }

   p_free_results(data->stmt_get_policy);


ENDE:
   close_prepared_statement(data->stmt_get_policy);

   return rc;
}


