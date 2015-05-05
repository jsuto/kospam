/*
 * avir.c, SJ
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <clapf.h>


int do_av_check(struct session_data *sdata, char *virusinfo, struct __data *data, struct __config *cfg){
   int rav = AVIR_OK;
   char avengine[SMALLBUFSIZE];

   if(sdata->need_scan == 0) return rav;

   memset(avengine, 0, sizeof(avengine));

#ifdef HAVE_LIBCLAMAV
   const char *virname;
   unsigned int options=0;

   options = CL_SCAN_STDOPT | CL_SCAN_ARCHIVE | CL_SCAN_MAIL | CL_SCAN_OLE2;

   if(cfg->use_libclamav_block_max_feature == 1) options |= CL_SCAN_BLOCKMAX;

   if(cfg->clamav_block_encrypted_archives == 1) options |= CL_SCAN_BLOCKENCRYPTED;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to pass to libclamav", sdata->ttmpfile);

   if(cl_scanfile(sdata->ttmpfile, &virname, NULL, data->engine, options) == CL_VIRUS){
      snprintf(virusinfo, SMALLBUFSIZE-1, "%s", virname);
      rav = AVIR_VIRUS;
      snprintf(avengine, sizeof(avengine)-1, "libClamAV");
   }

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: virus info: '%s'", sdata->ttmpfile, virname);
#endif

#ifdef HAVE_CLAMD
   if(strlen(cfg->clamd_addr) > 3 && cfg->clamd_port > 0){
      if(clamd_net_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
   } else {
      if(clamd_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
   }
#endif

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: done virus scanning", sdata->ttmpfile);

   return rav;
}


void update_bad_attachment_counter(struct session_data *sdata, char *digest){
   struct sql sql;

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_UPDATE_ADIGEST) == OK){

      p_bind_init(&sql);
      sql.sql[sql.pos] = digest; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

      p_exec_stmt(sdata, &sql);

      close_prepared_statement(&sql);
   }
}


int check_for_known_bad_attachments(struct session_data *sdata, struct __state *state){
   int i, counter=0, rc=AVIR_OK;
   struct sql sql;

   if(prepare_sql_statement(sdata, &sql, SQL_PREPARED_STMT_QUERY_ADIGEST) == ERR) return rc;

   for(i=1; i<=state->n_attachments; i++){
      syslog(LOG_PRIORITY, "%s: name=*%s*, type=%s, size=%d, digest=%s", sdata->ttmpfile, state->attachments[i].filename, state->attachments[i].type, state->attachments[i].size, state->attachments[i].digest);

      counter = 0;

      p_bind_init(&sql);
      sql.sql[sql.pos] = state->attachments[i].digest; sql.type[sql.pos] = TYPE_STRING; sql.pos++;

      if(p_exec_stmt(sdata, &sql) == ERR) goto CLOSE;

      p_bind_init(&sql);
      sql.sql[sql.pos] = (char *)&counter; sql.type[sql.pos] = TYPE_LONG; sql.len[sql.pos] = sizeof(int); sql.pos++;

      p_store_results(sdata, &sql);
      p_fetch_results(&sql);
      p_free_results(&sql);

      if(counter > 0){
         rc = AVIR_VIRUS;
         update_bad_attachment_counter(sdata, state->attachments[i].digest);
         syslog(LOG_PRIORITY, "%s: digest=%s is marked as malware (%d)", sdata->ttmpfile, state->attachments[i].digest, counter);
      }
   }


CLOSE:
   close_prepared_statement(&sql);

   return rc;
}


