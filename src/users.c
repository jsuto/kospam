/*
 * users.c, SJ
 */

#include <kospam.h>

int get_user_data_from_email(struct session_data *sdata, char *email, struct __config *cfg){
   int rc=0;
   char *p, *q, tmpbuf[SMALLBUFSIZE];
   struct sql sql;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: query user data for %s", sdata->ttmpfile, email);

   sdata->uid = 0;
   sdata->gid = 0;
   sdata->policy_group = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);
   memset(sdata->domain, 0, SMALLBUFSIZE);

   if(email == NULL) return rc;

   /* skip the +... part from the email */

   if((p = strchr(email, '+'))){
      *p = '\0';
      q = strchr(p+1, '@');

      if(!q) return 0;

      snprintf(tmpbuf, sizeof(tmpbuf)-1, "%s%s", email, q);
      *p = '+';
   }
   else snprintf(tmpbuf, sizeof(tmpbuf)-1, "%s", email);


   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_QUERY_USER_DATA) == ERR) return rc;

   p_bind_init(&sql);

   sql.sql[sql.pos] = &tmpbuf[0]; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

   if(p_exec_stmt(sdata, &sql) == ERR) goto ENDE;

   p_bind_init(&sql);

   sql.sql[sql.pos] = (char *)&(sdata->uid); sql.type[sql.pos] = TYPE_LONG; sql.pos++;
   sql.sql[sql.pos] = (char *)&(sdata->gid); sql.type[sql.pos] = TYPE_LONG; sql.pos++;
   sql.sql[sql.pos] = &(sdata->name[0]); sql.type[sql.pos] = TYPE_STRING; sql.len[sql.pos] = SMALLBUFSIZE-1; sql.pos++;
   sql.sql[sql.pos] = &(sdata->domain[0]); sql.type[sql.pos] = TYPE_STRING; sql.len[sql.pos] = SMALLBUFSIZE-1; sql.pos++;
   sql.sql[sql.pos] = (char *)&(sdata->policy_group); sql.type[sql.pos] = TYPE_LONG; sql.pos++;

   p_store_results(&sql);

   if(p_fetch_results(&sql) == OK){
      rc = 1;

      /* query white/black list data */
      get_wbl_data(sdata);
   }

   p_free_results(&sql);

ENDE:
   close_prepared_statement(&sql);

   return rc;
}


void get_wbl_data(struct session_data *sdata){
   char wh[MAXBUFSIZE], bl[MAXBUFSIZE];
   struct sql sql;

   memset(sdata->whitelist, 0, MAXBUFSIZE);
   memset(sdata->blacklist, 0, MAXBUFSIZE);

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_QUERY_WHITE_BLACK_LIST) == ERR) return;

   p_bind_init(&sql);

   sql.sql[sql.pos] = (char *)&(sdata->uid); sql.type[sql.pos] = TYPE_LONG; sql.pos++;

   if(p_exec_stmt(sdata, &sql) == ERR) goto ENDE;

   p_bind_init(&sql);

   sql.sql[sql.pos] = &wh[0]; sql.type[sql.pos] = TYPE_STRING; sql.len[sql.pos] = sizeof(wh)-1; sql.pos++;
   sql.sql[sql.pos] = &bl[0]; sql.type[sql.pos] = TYPE_STRING; sql.len[sql.pos] = sizeof(bl)-1; sql.pos++;

   p_store_results(&sql);

   while(p_fetch_results(&sql) == OK){

      if(!sql.is_null[0]){
         size_t len = strlen(sdata->whitelist);
         size_t rem = MAXBUFSIZE - len - 1;
         if(rem > 0){
            snprintf(sdata->whitelist + len, rem, "%s%s", len > 0 ? "\n" : "", wh);
         }
      }

      if(!sql.is_null[1]){
         size_t len = strlen(sdata->blacklist);
         size_t rem = MAXBUFSIZE - len - 1;
         if(rem > 0){
            snprintf(sdata->blacklist + len, rem, "%s%s", len > 0 ? "\n" : "", bl);
         }
      }

   }

   replace_character_in_buffer(sdata->whitelist, '\r', ',');
   replace_character_in_buffer(sdata->whitelist, '\n', ',');

   replace_character_in_buffer(sdata->blacklist, '\r', ',');
   replace_character_in_buffer(sdata->blacklist, '\n', ',');

   p_free_results(&sql);

ENDE:
   close_prepared_statement(&sql);
}
