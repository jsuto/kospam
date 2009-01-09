/*
 * smtp.c, 2009.01.09, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "smtpcodes.h"
#include "config.h"
#include "misc.h"
#include "defs.h"
#include "cfg.h"

/*
 * inject mail back to postfix
 */

int inject_mail(struct session_data *sdata, int msg, char *smtpaddr, int smtpport, char *spaminessbuf, struct __config *cfg, char *notify){
   int i, n, psd;
   char buf[MAXBUFSIZE+1], line[SMALLBUFSIZE], bigbuf[MAX_MAIL_HEADER_SIZE], oursigno[SMALLBUFSIZE];
   struct in_addr addr;
   struct sockaddr_in postfix_addr;
   struct timezone tz;
   struct timeval tv_start, tv_sent;
   int fd;
   int is_header=1, len=0, num_of_reads=0;
   int hdr_field_name_len, remove_hdr=0, remove_folded_hdr=0;
   int put_subject_spam_prefix =0, sent_subject_spam_prefix = 0;
   char *hdr_ptr, *p, *q, *recipient=NULL;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to inject back", sdata->ttmpfile);

   gettimeofday(&tv_start, &tz);


   memset(oursigno, 0, SMALLBUFSIZE);
   if(strlen(cfg->our_signo) > 3)
      snprintf(oursigno, SMALLBUFSIZE-1, "%s\r\n", cfg->our_signo);


   if(cfg->postfix_addr == NULL || smtpport == 0){
      syslog(LOG_PRIORITY, "%s: ERR: invalid smtp address or port", sdata->ttmpfile);
      return ERR_INJECT;
   }

   if((psd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      syslog(LOG_PRIORITY, "%s: ERR: create socket", sdata->ttmpfile);
      return ERR_INJECT;
   }

   postfix_addr.sin_family = AF_INET;
   postfix_addr.sin_port = htons(smtpport);
   inet_aton(smtpaddr, &addr);
   postfix_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(postfix_addr.sin_zero), 8);

   if(connect(psd, (struct sockaddr *)&postfix_addr, sizeof(struct sockaddr)) == -1){
      syslog(LOG_PRIORITY, "%s: ERR: connect to %s %d", sdata->ttmpfile, smtpaddr, smtpport);
      return ERR_INJECT;
   }

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got in injecting: %s", sdata->ttmpfile, buf);

   /* 220 banner */

   if(strncmp(buf, "220", 3)){
      send(psd, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT), 0);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, SMTP_CMD_QUIT);
      close(psd);
      syslog(LOG_PRIORITY, "%s: missing 220 banner (%s)", sdata->ttmpfile, buf);
      return ERR_INJECT;
   }

   snprintf(buf, MAXBUFSIZE-1, "HELO %s\r\n", cfg->hostid);
   send(psd, buf, strlen(buf), 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, buf);

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got in injecting: %s", sdata->ttmpfile, buf);

   /* HELO */

   if(strncmp(buf, "250", 3)){
      send(psd, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT), 0);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, SMTP_CMD_QUIT);
      close(psd);
      syslog(LOG_PRIORITY, "%s: failed HELO (%s)", sdata->ttmpfile, buf);
      return ERR_INJECT;
   }

   /* MAIL FROM */
 
   send(psd, sdata->mailfrom, strlen(sdata->mailfrom), 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, sdata->mailfrom);

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got in injecting: %s", sdata->ttmpfile, buf);

   if(strncmp(buf, "250", 3)){
      send(psd, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT), 0);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, SMTP_CMD_QUIT);
      close(psd);
      syslog(LOG_PRIORITY, "%s: MAIL FROM failed (%s)", sdata->ttmpfile, buf);
      if(strncmp(buf, "550", 3) == 0) return ERR_REJECT;
      return ERR_INJECT;
   }

   /* RCPT TO */

#ifndef HAVE_LMTP
   for(i=0; i<sdata->num_of_rcpt_to; i++){
#else
      i = msg;
#endif
      recipient = sdata->rcptto[i];

      send(psd, sdata->rcptto[i], strlen(sdata->rcptto[i]), 0);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting (%d): %s", sdata->ttmpfile, i, sdata->rcptto[i]);

      n = recvtimeout(psd, buf, MAXBUFSIZE, 0);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got in injecting (%d): %s", sdata->ttmpfile, i, buf);

      if(strncmp(buf, "250", 3)){
         send(psd, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT), 0);
         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting (%d): %s", sdata->ttmpfile, i, SMTP_CMD_QUIT);
         close(psd);
         syslog(LOG_PRIORITY, "%s: RCPT TO (%d) failed (%s)", sdata->ttmpfile, i, buf);
         if(strncmp(buf, "550", 3) == 0) return ERR_REJECT;
         return ERR_INJECT;
      }
#ifndef HAVE_LMTP
   }
#endif


   /* DATA */

   send(psd, SMTP_CMD_DATA, strlen(SMTP_CMD_DATA), 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, SMTP_CMD_DATA);

   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got in injecting: %s", sdata->ttmpfile, buf);

   if(strncmp(buf, "354", 3)){
      send(psd, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT), 0);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, SMTP_CMD_QUIT);
      syslog(LOG_PRIORITY, "%s: DATA failed (%s)", sdata->ttmpfile, buf);
      close(psd);
      return ERR_INJECT;
   }

   /* send data */

   if(notify == NULL){
      /* read and send stored mail */

      fd = open(sdata->ttmpfile, O_RDONLY);
      if(fd == -1)
         return ERR_INJECT;

      /* is this message spam and do we have to put [sp@m] prefix to the Subject: line? */

      if(spaminessbuf && strlen(cfg->spam_subject_prefix) > 22 && strstr(spaminessbuf, cfg->clapf_spam_header_field))
         put_subject_spam_prefix = 1;

      /*
        the header_size_limit variable of postfix (see the output of postconf)
        limits the length of the message header. So we read a big one and it
        will contain the whole header. Now we can rewrite the necessary header
        lines. 2006.08.21, SJ
      */

      while((n = read(fd, bigbuf, MAX_MAIL_HEADER_SIZE)) > 0){
         num_of_reads++;

         /* if this is the first read and it contains all the header lines */

         if(num_of_reads == 1){

            /* first find the end of the mail header */

            for(i=0; i<n; i++){
               if( (bigbuf[i] == '\r' && bigbuf[i+1] == '\n' && bigbuf[i+2] == '\r' && bigbuf[i+3] == '\n') ||
                   (bigbuf[i] == '\n' && bigbuf[i+1] == '\n') ){
                  is_header = 0;
                  break;
               }
            }

            if(is_header == 0){
               p = bigbuf;
               
               while((q = strchr(p, '\n')) && len < i+1){
                  *q = '\0';
                  snprintf(line, SMALLBUFSIZE-1, "%s\n", p);

                  /* Mariano Reingart gave the following idea and code snippet */

                  if(line[0] == ' ' || line[0] == '\t'){
                     remove_hdr = remove_folded_hdr; /* LWSP Folded header */
                  }
                  else {
                     remove_folded_hdr = 0;
                     hdr_field_name_len = strcspn(p, ": \t\n\r");
                     if(hdr_field_name_len){ /* got a valid header */
                        hdr_ptr = spaminessbuf;
                        while(hdr_ptr){
                           /* drop header because it is in spaminessbuf */
                           if(!strncasecmp(line, hdr_ptr, hdr_field_name_len)){
                              remove_hdr = 1;
                              remove_folded_hdr = 1;
                              break;
                           }

                           hdr_ptr = strstr(hdr_ptr, "\r\n");
                           if(hdr_ptr){
                              hdr_ptr += 2; /* skip CRLF to the next header */
                              if(hdr_ptr[0] == '\0') hdr_ptr = NULL; /* set null if we reached end of spaminessbuf */
                           }
                        }
                     }
                  }

                  if(!remove_hdr){

                     /* send the spam_subject_prefix in the Subject line if this is a spam, 2006.11.13, SJ */

                     if(put_subject_spam_prefix == 1 && strncmp(line, "Subject: ", 9) == 0 && !strstr(line, cfg->spam_subject_prefix) ){
                        send(psd, "Subject: ", 9, 0);
                        send(psd, cfg->spam_subject_prefix, strlen(cfg->spam_subject_prefix), 0);
                        send(psd, line+9, strlen(line)-9, 0);
                        sent_subject_spam_prefix = 1;
                     }
                     else
                        send(psd, line, strlen(line), 0);
                  }
                  else
                     remove_hdr = 0;

                  len += strlen(p) + 1;
                  p = q + 1;
               }

               /* if no Subject: line but this is a spam, create a Subject: line, 2006.11.13, SJ */

               if(put_subject_spam_prefix == 1 && sent_subject_spam_prefix == 0){
                  snprintf(line, SMALLBUFSIZE-1, "Subject: %s\r\n", cfg->spam_subject_prefix);
                  send(psd, line, strlen(line), 0);
               }

               /* send spaminessbuf and the rest */

               if(strlen(oursigno) > 3 && is_recipient_in_our_domains(recipient, cfg) == 0)
                  send(psd, oursigno, strlen(oursigno), 0);

               if(spaminessbuf)
                  send(psd, spaminessbuf, strlen(spaminessbuf), 0);


               send(psd, bigbuf+i+2, n-i-2, 0);
            }
            else {
               /* or send stuff as we can as a last resort */

               if(strlen(oursigno) > 3 && is_recipient_in_our_domains(recipient, cfg) == 0)
                  send(psd, oursigno, strlen(oursigno), 0);

               if(spaminessbuf)
                  send(psd, spaminessbuf, strlen(spaminessbuf), 0);

               /* do not bother with sending the spam_subject_prefix in the Subject line for now 
                  if we can't find the endof the header, 2006.11.13, SJ */

               send(psd, bigbuf, n, 0);
            }


         } /* end of first read */
         else
            send(psd, bigbuf, n, 0);

      }

      close(fd);
   }
   else {
      /* or send notification about the virus found */
      send(psd, notify, strlen(notify), 0);
   }


   n = recvtimeout(psd, buf, MAXBUFSIZE, 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got in injecting: %s", sdata->ttmpfile, buf);

   buf[n-2] = '\0';

   gettimeofday(&tv_sent, &tz);

   if(strncmp(buf, "250", 3) == 0)
      /* our mail was accepted */
      syslog(LOG_PRIORITY, "%s: injected back, sent: %ld [ms] -> %s", sdata->ttmpfile, tvdiff(tv_sent, tv_start)/1000, buf);

   else
      /* our messages was not rejected with 550 but inject failed */
      syslog(LOG_PRIORITY, "%s: injecting failed (%s)", sdata->ttmpfile, buf);


   send(psd, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT), 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, SMTP_CMD_QUIT);

   close(psd);

   if(strncmp(buf, "250", 3) == 0)
      return OK;
   else if(strncmp(buf, "550", 3) == 0)
      return ERR_REJECT;
   else
      return ERR_INJECT;
}

