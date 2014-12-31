/*
 * users.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <clapf.h>


int get_user_data_from_email(struct session_data *sdata, struct __data *data, char *email, struct __config *cfg){
   int rc=0;
   char *p, *q, tmpbuf[SMALLBUFSIZE];

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


   if(prepare_sql_statement(sdata, &(data->stmt_get_user_data), SQL_PREPARED_STMT_QUERY_USER_DATA) == ERR) return rc;

   p_bind_init(data);

   data->sql[data->pos] = &tmpbuf[0]; data->type[data->pos] = TYPE_STRING; data->pos++;

   if(p_exec_query(sdata, data->stmt_get_user_data, data) == ERR) goto ENDE;

   p_bind_init(data);

   data->sql[data->pos] = (char *)&(sdata->uid); data->type[data->pos] = TYPE_LONG; data->pos++;
   data->sql[data->pos] = (char *)&(sdata->gid); data->type[data->pos] = TYPE_LONG; data->pos++;
   data->sql[data->pos] = &(sdata->name[0]); data->type[data->pos] = TYPE_STRING; data->len[data->pos] = SMALLBUFSIZE-1; data->pos++;
   data->sql[data->pos] = &(sdata->domain[0]); data->type[data->pos] = TYPE_STRING; data->len[data->pos] = SMALLBUFSIZE-1; data->pos++;
   data->sql[data->pos] = (char *)&(sdata->policy_group); data->type[data->pos] = TYPE_LONG; data->pos++;

   p_store_results(sdata, data->stmt_get_user_data, data);

   if(p_fetch_results(data->stmt_get_user_data) == OK){
      rc = 1;

      /* query white/black list data */
      get_wbl_data(sdata, data, cfg);
   }

   p_free_results(data->stmt_get_user_data);

ENDE:
   close_prepared_statement(data->stmt_get_user_data);

   return rc;
}


void get_wbl_data(struct session_data *sdata, struct __data *data, struct __config *cfg){
   char wh[MAXBUFSIZE], bl[MAXBUFSIZE];

   memset(sdata->whitelist, 0, MAXBUFSIZE);
   memset(sdata->blacklist, 0, MAXBUFSIZE);

   if(prepare_sql_statement(sdata, &(data->stmt_generic), SQL_PREPARED_STMT_QUERY_WHITE_BLACK_LIST) == ERR) return;

   p_bind_init(data);

   data->sql[data->pos] = (char *)&(sdata->uid); data->type[data->pos] = TYPE_LONG; data->pos++;

   if(p_exec_query(sdata, data->stmt_generic, data) == ERR) goto ENDE;

   p_bind_init(data);

   data->sql[data->pos] = &wh[0]; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = sizeof(wh)-1; data->pos++;
   data->sql[data->pos] = &bl[0]; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = sizeof(bl)-1; data->pos++;

   p_store_results(sdata, data->stmt_generic, data);

   while(p_fetch_results(data->stmt_generic) == OK){

      if(strlen(sdata->whitelist) > 0) strncat(sdata->whitelist, "\n", MAXBUFSIZE-1);
      strncat(sdata->whitelist, wh, MAXBUFSIZE-1);

      if(strlen(sdata->blacklist) > 0) strncat(sdata->blacklist, "\n", MAXBUFSIZE-1);
      strncat(sdata->blacklist, bl, MAXBUFSIZE-1);
   }

   replace_character_in_buffer(sdata->whitelist, '\r', ',');
   replace_character_in_buffer(sdata->whitelist, '\n', ',');

   replace_character_in_buffer(sdata->blacklist, '\r', ',');
   replace_character_in_buffer(sdata->blacklist, '\n', ',');

   p_free_results(data->stmt_generic);

ENDE:
   close_prepared_statement(data->stmt_generic);
}



