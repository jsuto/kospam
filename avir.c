/*
 * avir.c, 2009.05.27, SJ
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "defs.h"
#include "misc.h"
#include "av.h"
#include "templates.h"
#include "session.h"
#include "config.h"

#ifdef HAVE_LIBCLAMAV
int do_av_check(struct session_data *sdata, char *email, char *email2, struct cl_engine *engine, struct __config *cfg){
#else
int do_av_check(struct session_data *sdata, char *email, char *email2, struct __config *cfg){
#endif
   int ret, rav = AVIR_OK; /* antivirus result is ok by default */
   struct timezone tz;
   struct timeval tv_rcvd, tv_scnd;
   char buf[MAXBUFSIZE], avengine[SMALLBUFSIZE], virusinfo[SMALLBUFSIZE];

   if(sdata->need_scan == 0) return rav;

   memset(avengine, 0, SMALLBUFSIZE);

   gettimeofday(&tv_rcvd, &tz);

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
   if(avast_scan(sdata.ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_KAV
   if(kav_scan(sdata.ttmpfile, avengine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_DRWEB
   if(drweb_scan(sdata.ttmpfile, avengine, virusinfo, &cfg) == AV_VIRUS) rav = AVIR_VIRUS;
#endif

#ifdef HAVE_CLAMD
   if(strlen(cfg->clamd_addr) > 3){
      if(clamd_net_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
   } else {
      if(clamd_scan(sdata->ttmpfile, avengine, virusinfo, cfg) == AV_VIRUS) rav = AVIR_VIRUS;
   }
#endif

   gettimeofday(&tv_scnd, &tz);

   syslog(LOG_PRIORITY, "%s: virus scanning done in %ld [ms]", sdata->ttmpfile, tvdiff(tv_scnd, tv_rcvd)/1000);


   /* if a virus has found */

   if(rav == AVIR_VIRUS){
      syslog(LOG_PRIORITY, "%s: Virus found %s", sdata->ttmpfile, virusinfo);

      /* move to quarantine, if we have to */

      if(strlen(cfg->quarantine_dir) > 3)
         move_message_to_quarantine(sdata, cfg->quarantine_dir);

      /* send notification if localpostmaster is set, 2005.10.04, SJ */

      if(strlen(cfg->localpostmaster) > 3){

         memset(email, 0, SMALLBUFSIZE);
         extract_email(sdata->rcptto[0], email);

         if(get_template(VIRUS_TEMPLATE, buf, cfg->localpostmaster, email, email2, virusinfo, avengine) == 1){

            snprintf(sdata->rcptto[0], SMALLBUFSIZE-1, "RCPT TO: <%s>\r\n", cfg->localpostmaster);
            sdata->num_of_rcpt_to = 1;
            ret = inject_mail(sdata, 0, cfg->postfix_addr, cfg->postfix_port, NULL, cfg, buf);

            if(ret == 0)
               syslog(LOG_PRIORITY, "notification about %s to %s failed", sdata->ttmpfile, cfg->localpostmaster);
         }
      }
   }


   return rav;
}
