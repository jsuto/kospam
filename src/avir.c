/*
 * avir.c, 2010.05.13, SJ
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "defs.h"
#include "misc.h"
#include "av.h"
#include "templates.h"
#include "session.h"
#include "config.h"

#ifdef HAVE_LIBCLAMAV
int do_av_check(struct session_data *sdata, char *email, char *email2, char *virusinfo, struct cl_engine *engine, struct __config *cfg){
#else
int do_av_check(struct session_data *sdata, char *email, char *email2, char *virusinfo, struct __config *cfg){
#endif
   int ret, rav = AVIR_OK; /* antivirus result is ok by default */
   char buf[MAXBUFSIZE], avengine[SMALLBUFSIZE];

   if(sdata->need_scan == 0) return rav;

   memset(avengine, 0, SMALLBUFSIZE);

#ifdef HAVE_LIBCLAMAV
   const char *virname;
   unsigned int options=0;

   options = CL_SCAN_STDOPT | CL_SCAN_ARCHIVE | CL_SCAN_MAIL | CL_SCAN_OLE2;

   /* whether to mark archives as viruses if maxfiles, maxfilesize, or maxreclevel limit is reached */
   if(cfg->use_libclamav_block_max_feature == 1) options |= CL_SCAN_BLOCKMAX;

   /* whether to mark encrypted archives as viruses */
   if(cfg->clamav_block_encrypted_archives == 1) options |= CL_SCAN_BLOCKENCRYPTED;

   ret = cl_scanfile(sdata->ttmpfile, &virname, NULL, engine, options);

   if(ret == CL_VIRUS){
      memset(virusinfo, 0, SMALLBUFSIZE);
      strncpy(virusinfo, virname, SMALLBUFSIZE-1);
      rav = AVIR_VIRUS;
      snprintf(avengine, SMALLBUFSIZE-1, "libClamAV");
   }
#endif

#ifdef HAVE_AVAST
   if(avast_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_AVAST_HOME
   if(avast_cmd_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_KAV
   if(kav_scan(sdata->ttmpfile, avengine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_DRWEB
   if(drweb_scan(sdata->ttmpfile, avengine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_CLAMD
   if(strlen(cfg->clamd_addr) > 3 && cfg->clamd_port > 0){
      if(clamd_net_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
   } else {
      if(clamd_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
   }
#endif

   /* if a virus has found */

   if(rav == AVIR_VIRUS){

      if(strlen(cfg->quarantine_dir) > 3) moveMessageToQuarantine(sdata, cfg);

      /* send notification if localpostmaster is set, 2005.10.04, SJ */

      if(strlen(cfg->localpostmaster) > 3){

         memset(email, 0, SMALLBUFSIZE);
         extractEmail(sdata->rcptto[0], email);

         if(createEmailFromTemplate(VIRUS_TEMPLATE, buf, cfg->localpostmaster, email, email2, virusinfo, avengine) == 1){

            snprintf(sdata->rcptto[0], SMALLBUFSIZE-1, "RCPT TO: <%s>\r\n", cfg->localpostmaster);
            sdata->num_of_rcpt_to = 1;
            ret = inject_mail(sdata, 0, cfg->postfix_addr, cfg->postfix_port, NULL, &buf[0], cfg, buf);

            if(ret == 0)
               syslog(LOG_PRIORITY, "notification about %s to %s failed", sdata->ttmpfile, cfg->localpostmaster);
         }
      }
   }


   return rav;
}


/*
 * move message to quarantine
 */

int moveMessageToQuarantine(struct session_data *sdata, struct __config *cfg){
   char qfile[QUARANTINELEN];

   snprintf(qfile, QUARANTINELEN-1, "%s/%s", cfg->quarantine_dir, sdata->ttmpfile);

   /* if we would use rename, we cannot unlink this file later, producing an
      error message to syslog */

   if(link(sdata->ttmpfile, qfile) == 0){
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s saved as %s", sdata->ttmpfile, qfile);
      chmod(qfile, 0644);

      return OK;
   }
   else {
      syslog(LOG_PRIORITY, "failed to put %s into quarantine: %s", sdata->ttmpfile, qfile);
      return ERR;
   }

}

