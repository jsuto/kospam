/*
 * policy.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <clapf.h>


int get_policy(struct session_data *sdata, struct __config *cfg, struct __config *my_cfg){
   int rc=0;
   float f;
   char f1[MAXVAL], f2[MAXVAL];
   struct sql sql;

   /*
    * in case of smtp mode don't query unless we have a single recipient
    */

   if(cfg->server_mode == SMTP_MODE && sdata->num_of_rcpt_to != 1) return rc;

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_QUERY_POLICY) == ERR) return rc;

   p_bind_init(&sql);

   sql.sql[sql.pos] = (char *)&(sdata->policy_group); sql.type[sql.pos] = TYPE_LONG; sql.pos++;

   if(p_exec_stmt(sdata, &sql) == ERR) goto ENDE;

   p_bind_init(&sql);

   sql.sql[sql.pos] = (char *)&(my_cfg->deliver_infected_email); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->silently_discard_infected_email); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->use_antispam); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = my_cfg->spam_subject_prefix; sql.type[sql.pos] = TYPE_STRING; sql.len[sql.pos] = MAXVAL-1; sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->max_message_size_to_filter); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = my_cfg->surbl_domain; sql.type[sql.pos] = TYPE_STRING; sql.len[sql.pos] = MAXVAL-1; sql.pos++;
   sql.sql[sql.pos] = &f1[0]; sql.type[sql.pos] = TYPE_STRING; sql.len[sql.pos] = MAXVAL-1; sql.pos++;
   sql.sql[sql.pos] = &f2[0]; sql.type[sql.pos] = TYPE_STRING; sql.len[sql.pos] = MAXVAL-1; sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->replace_junk_characters); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->penalize_images); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->penalize_embed_images); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->penalize_octet_stream); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->training_mode); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->store_emails); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->store_only_spam); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = (char *)&(my_cfg->message_from_a_zombie); sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;
   sql.sql[sql.pos] = my_cfg->smtp_addr; sql.type[sql.pos] = TYPE_STRING; sql.len[sql.pos] = MAXVAL-1; sql.pos++;


   p_store_results(&sql);

   if(p_fetch_results(&sql) == OK){
      f = atof(f1); if(f > 0.1) my_cfg->spam_overall_limit = f;
      f = atof(f2); if(f > 0.1) my_cfg->spaminess_oblivion_limit = f;

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: policy settings: %d/%d/%d/%s/%d/%s/%.4f/%.4f/%d/%d/%d/%d/%d/%d/%d/%d/%s",
                                              sdata->ttmpfile, my_cfg->deliver_infected_email, my_cfg->silently_discard_infected_email, my_cfg->use_antispam,
                                              my_cfg->spam_subject_prefix, my_cfg->max_message_size_to_filter, my_cfg->surbl_domain, my_cfg->spam_overall_limit,
                                              my_cfg->spaminess_oblivion_limit, my_cfg->replace_junk_characters, my_cfg->penalize_images, my_cfg->penalize_embed_images,
                                              my_cfg->penalize_octet_stream, my_cfg->training_mode, my_cfg->store_emails, my_cfg->store_only_spam, my_cfg->message_from_a_zombie,
                                              my_cfg->smtp_addr);
   }

   p_free_results(&sql);


ENDE:
   close_prepared_statement(&sql);

   return rc;
}
