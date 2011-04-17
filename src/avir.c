/*
 * avir.c, SJ
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <clapf.h>


int do_av_check(struct session_data *sdata, char *rcpttoemail, char *fromemail, char *virusinfo, struct __data *data, struct __config *cfg){
   int rav = AVIR_OK;
   char avengine[SMALLBUFSIZE];

   if(sdata->need_scan == 0) return rav;

   memset(avengine, 0, SMALLBUFSIZE);

#ifdef HAVE_LIBCLAMAV
   const char *virname;
   unsigned int options=0;

   options = CL_SCAN_STDOPT | CL_SCAN_ARCHIVE | CL_SCAN_MAIL | CL_SCAN_OLE2;

   if(cfg->use_libclamav_block_max_feature == 1) options |= CL_SCAN_BLOCKMAX;

   if(cfg->clamav_block_encrypted_archives == 1) options |= CL_SCAN_BLOCKENCRYPTED;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to pass to libclamav", sdata->ttmpfile);

   memset(virusinfo, 0, SMALLBUFSIZE);

   if(cl_scanfile(sdata->ttmpfile, &virname, NULL, data->engine, options) == CL_VIRUS){
      strncpy(virusinfo, virname, SMALLBUFSIZE-1);
      rav = AVIR_VIRUS;
      snprintf(avengine, SMALLBUFSIZE-1, "libClamAV");
   }

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: virus info: '%s'", sdata->ttmpfile, virusinfo);
#endif

#ifdef HAVE_AVAST
   if(avast_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_AVAST_HOME
   if(avast_cmd_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_KAV
   if(kav_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_DRWEB
   if(drweb_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_CLAMD
   if(strlen(cfg->clamd_addr) > 3 && cfg->clamd_port > 0){
      if(clamd_net_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
   } else {
      if(clamd_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
   }
#endif

   if(rav == AVIR_VIRUS){
      if(strlen(cfg->quarantine_dir) > 3) moveMessageToQuarantine(sdata, cfg);
      if(strlen(cfg->localpostmaster) > 3) sendNotificationToPostmaster(sdata, rcpttoemail, fromemail, virusinfo, avengine, cfg);
   }


   return rav;
}


int moveMessageToQuarantine(struct session_data *sdata, struct __config *cfg){
   char qfile[QUARANTINELEN];

   snprintf(qfile, QUARANTINELEN-1, "%s/%s", cfg->quarantine_dir, sdata->ttmpfile);

   if(link(sdata->ttmpfile, qfile) == 0){
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: saved as %s", sdata->ttmpfile, qfile);
      chmod(qfile, 0644);

      return OK;
   }
   else {
      syslog(LOG_PRIORITY, "%s: failed to move into quarantine: %s", sdata->ttmpfile, qfile);
      return ERR;
   }

}


void sendNotificationToPostmaster(struct session_data *sdata, char *rcpttoemail, char *fromemail, char *virusinfo, char *avengine, struct __config *cfg){
   int ret;
   char buf[MAXBUFSIZE], notify[MAXBUFSIZE];

   memset(rcpttoemail, 0, SMALLBUFSIZE);
   extractEmail(sdata->rcptto[0], rcpttoemail);

   if(createMessageFromTemplate(VIRUS_TEMPLATE, &notify[0], cfg->localpostmaster, rcpttoemail, fromemail, virusinfo, avengine) == 1){

      snprintf(sdata->rcptto[0], SMALLBUFSIZE-1, "RCPT TO: <%s>\r\n", cfg->localpostmaster);
      sdata->num_of_rcpt_to = 1;

      ret = inject_mail(sdata, 0, cfg->postfix_addr, cfg->postfix_port, NULL, &buf[0], cfg, notify);

      if(ret != OK)
         syslog(LOG_PRIORITY, "%s: notification failed to %s", sdata->ttmpfile, cfg->localpostmaster);
   }

}

