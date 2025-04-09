/*
 * history.c, SJ
 */

#include <kospam.h>


int store_file_to_quarantine(char *filename, struct __config *cfg){
   int rc=ERR;
   char *p0, *p1, *p2, s[SMALLBUFSIZE];
   struct stat st;

   /* create a filename in the store based on piler_id */

   snprintf(s, sizeof(s)-1, "%s/%02x/%c%c/%c%c/%s", cfg->quarantinedir, cfg->server_id, filename[RND_STR_LEN-4], filename[RND_STR_LEN-3], filename[RND_STR_LEN-2], filename[RND_STR_LEN-1], filename);

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

int write_history_to_sql(struct session_data *sdata, struct __state *state){
   int rc=ERR;
   struct sql sql;

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_INSERT_INTO_HISTORY) == ERR) return rc;

   p_bind_init(&sql);

   char subject[TINYBUFSIZE];
   snprintf(subject, sizeof(subject)-1, "%s", state->b_subject);

   sql.sql[sql.pos] = (char *)&(sdata->now); sql.type[sql.pos] = TYPE_LONG; sql.pos++;
   sql.sql[sql.pos] = sdata->ttmpfile; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = (char *)&(sdata->status); sql.type[sql.pos] = TYPE_LONG; sql.pos++;
   sql.sql[sql.pos] = state->fromemail; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = &subject[0]; sql.type[sql.pos] = TYPE_STRING; sql.pos++;
   sql.sql[sql.pos] = (char *)&(sdata->tot_len); sql.type[sql.pos] = TYPE_LONG; sql.pos++;

   if(p_exec_stmt(sdata, &sql) == OK) rc = OK;

   close_prepared_statement(&sql);

   return rc;
}


int write_history_to_fs(struct session_data *sdata, struct __state *state, char *status, struct __config *cfg){
   int i, rc=ERR, fd, len=0;
   char tmpname[SMALLBUFSIZE], name[SMALLBUFSIZE], buf[SMALLBUFSIZE], recipient[SMALLBUFSIZE];
   Bytef *z=NULL;
   uLongf dstlen;

   for(i=0; i<sdata->num_of_rcpt_to; i++){

      snprintf(recipient, sizeof(recipient)-1, "%s", sdata->rcptto[i]);
      extract_verp_address(recipient);

      snprintf(tmpname, sizeof(tmpname)-1, "%s/tmp/%s", HISTORY_DIR, sdata->ttmpfile);
      snprintf(name, sizeof(name)-1, "%s/new/%s", HISTORY_DIR, sdata->ttmpfile);

      snprintf(buf, sizeof(buf)-1, "%s%c%ld%c%s%c%s%c%d%c%d%c%s:%c%s%c%s", sdata->ttmpfile, DELIM, sdata->now, DELIM, state->fromemail, DELIM, recipient, DELIM, sdata->tot_len, DELIM, sdata->status, DELIM, cfg->smtp_addr, DELIM, status, DELIM, state->b_subject);

      len = strlen(buf);
      dstlen = compressBound(len);
      z = malloc(dstlen);
      if(!z) continue;

      rc = compress(z, &dstlen, (const Bytef *)&buf[0], len);

      fd = open(tmpname, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP);
      if(fd == -1){
         syslog(LOG_PRIORITY, "%s: error: cannot open '%s'", sdata->ttmpfile, tmpname);
      }
      else {
         if(write(fd, z, dstlen) == -1) syslog(LOG_PRIORITY, "%s: error: write()", sdata->ttmpfile);
         close(fd);

         if(!rename(tmpname, name)) rc = OK;
      }

      free(z);
   }

   return rc;
}


int write_history(struct session_data *sdata, struct __state *state, char *status, struct __config *cfg){

   if(cfg->store_emails == 1 && (cfg->store_only_spam == 0 || sdata->spaminess >= cfg->spam_overall_limit) ) store_file_to_quarantine(sdata->ttmpfile, cfg);

   if(cfg->history == 0)
      return write_history_to_sql(sdata, state);
   else
      return write_history_to_fs(sdata, state, status, cfg);
}
