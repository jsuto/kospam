/*
 * quarantine.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <clapf.h>


int store_file_to_quarantine(char *filename, struct __config *cfg){
   int rc=ERR;
   char *p0, *p1, *p2, s[SMALLBUFSIZE];
   struct stat st;

   /* create a filename in the store based on piler_id */

   snprintf(s, sizeof(s)-1, "%s/%02x/%c%c/%c%c/%s", cfg->queuedir, cfg->server_id, filename[RND_STR_LEN-4], filename[RND_STR_LEN-3], filename[RND_STR_LEN-2], filename[RND_STR_LEN-1], filename);

   p0 = strrchr(s, '/'); if(!p0) return rc;
   *p0 = '\0';

   if(stat(s, &st)){
      p1 = strrchr(s, '/'); if(!p1) return rc;
      *p1 = '\0';
      p2 = strrchr(s, '/'); if(!p2) return rc;
      *p2 = '\0';

      rc = mkdir(s, 0755);
      *p2 = '/';
      rc = mkdir(s, 0755);
      *p1 = '/';
      rc = mkdir(s, 0755); if(rc == -1) syslog(LOG_PRIORITY, "%s: mkdir %s: error=%s", filename, s, strerror(errno));
   }

   *p0 = '/';

   rc = ERR;

   if(link(filename, s) == 0) rc = OK;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: saving to queue: %s", filename, s);

   return rc;
}


int write_history(struct session_data *sdata, struct __state *state, struct __data *data, char *status, struct __config *cfg){
   int i, rc = 0; 
   char relay[SMALLBUFSIZE];

   if(cfg->store_emails == 1 && (cfg->store_only_spam == 0 || sdata->spaminess < cfg->spam_overall_limit) ) store_file_to_quarantine(sdata->ttmpfile, cfg);

   if(prepare_sql_statement(sdata, &(data->stmt_insert_into_history), SQL_PREPARED_STMT_INSERT_INTO_HISTORY) == ERR) return rc;

   snprintf(relay, sizeof(relay)-1, "%s:%d", cfg->smtp_addr, cfg->smtp_port);

   for(i=0; i<sdata->num_of_rcpt_to; i++){
      p_bind_init(data);

      data->sql[data->pos] = (char *)&(sdata->now); data->type[data->pos] = TYPE_LONG; data->pos++;
      data->sql[data->pos] = sdata->ttmpfile; data->type[data->pos] = TYPE_STRING; data->pos++;
      data->sql[data->pos] = (char *)&(sdata->status); data->type[data->pos] = TYPE_LONG; data->pos++;
      data->sql[data->pos] = sdata->fromemail; data->type[data->pos] = TYPE_STRING; data->pos++;
      data->sql[data->pos] = sdata->rcptto[i]; data->type[data->pos] = TYPE_STRING; data->pos++;
      data->sql[data->pos] = state->b_subject; data->type[data->pos] = TYPE_STRING; data->pos++;
      data->sql[data->pos] = (char *)&(sdata->tot_len); data->type[data->pos] = TYPE_LONG; data->pos++;
      data->sql[data->pos] = (char *)&(state->n_attachments); data->type[data->pos] = TYPE_LONG; data->pos++;
      data->sql[data->pos] = &relay[0]; data->type[data->pos] = TYPE_STRING; data->pos++;
      data->sql[data->pos] = status; data->type[data->pos] = TYPE_STRING; data->pos++;

      if(p_exec_query(sdata, data->stmt_insert_into_history, data) == OK) rc = 1; 
   }


   mysql_stmt_close(data->stmt_insert_into_history);

   return rc;
}


int create_partition(struct __config *cfg){
   int rc=ERR;
   char buf[SMALLBUFSIZE];
   struct tm *t;
   struct session_data sdata;

   init_session_data(&sdata, cfg);

   t = localtime(&(sdata.now));

   snprintf(buf, sizeof(buf)-1, "ALTER TABLE `%s` ADD PARTITION ( PARTITION p%d%02d%02d VALUES LESS THAN (%ld) )", SQL_HISTORY_TABLE, t->tm_year+1900, t->tm_mon+1, t->tm_mday, sdata.now + 86400);

   if(open_database(&sdata, cfg) == OK){
      syslog(LOG_PRIORITY, "partition query: %s", buf);
      p_query(&sdata, buf);
      close_database(&sdata);

      rc = OK;
   }

   return rc;
}


int drop_partition(struct __config *cfg){
   int rc=ERR;
   char buf[SMALLBUFSIZE];
   struct tm *t;
   struct session_data sdata;

   init_session_data(&sdata, cfg);

   sdata.now -= 31*86400;
   t = localtime(&(sdata.now));

   snprintf(buf, sizeof(buf)-1, "ALTER TABLE `%s` DROP PARTITION p%d%02d%02d", SQL_HISTORY_TABLE, t->tm_year+1900, t->tm_mon+1, t->tm_mday);

   if(open_database(&sdata, cfg) == OK){
      syslog(LOG_PRIORITY, "partition query: %s", buf);
      p_query(&sdata, buf);
      close_database(&sdata);

      rc = OK;
   }

   return rc;
}

