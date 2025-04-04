/*
 * smtp.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <clapf.h>


int send_headers(int sd, char *bigbuf, int n, char *spaminessbuf, int put_subject_spam_prefix, int put_subject_possible_spam_prefix, struct session_data *sdata, struct __config *cfg){
   int i=0, result, x, N, is_header=1, remove_hdr=0, remove_folded_hdr=0, hdr_field_name_len, sent_subject_spam_prefix=0, has_subject=0;
   char *p, *q, *hdr_ptr, buf[MAXBUFSIZE], headerbuf[MAX_MAIL_HEADER_SIZE+SMALLBUFSIZE];

   /* any reasonable message really should be longer than 20 bytes */
   if(n < 20) return 0;

   memset(headerbuf, 0, sizeof(headerbuf));

   /*
    * prepend the clapf id to the mail header. This obsoletes
    * the Outlook hack, and required for store-less training.
    */

   snprintf(headerbuf, sizeof(headerbuf)-1, "Received: %s\r\n", sdata->ttmpfile);


   /* first find the end of the mail header */

   x = search_string_in_buffer(bigbuf, n, "\r\n.\r\n", 5);
   if(x > 0) N = x;
   else N = n;

   for(i=5; i<N-3; i++){
      if( (bigbuf[i] == '\r' && bigbuf[i+1] == '\n' && bigbuf[i+2] == '\r' && bigbuf[i+3] == '\n') ||
          (bigbuf[i] == '\n' && bigbuf[i+1] == '\n') ){
              is_header = 0;
              break;
      }
   }



   p = bigbuf;
   q = p + i;


   /* parse header lines */

   do {
      p = split(p, '\n', buf, sizeof(buf)-1, &result);

      if(buf[0] == ' ' || buf[0] == '\t'){
         remove_hdr = remove_folded_hdr; /* LWSP Folded header */
      }
      else {
         remove_folded_hdr = 0;
         hdr_field_name_len = strcspn(buf, ": \t\n\r");
         if(hdr_field_name_len){ /* got a valid header */
            hdr_ptr = spaminessbuf;
            while(hdr_ptr){
               /* drop header because it is in spaminessbuf */
               if(!strncasecmp(buf, hdr_ptr, hdr_field_name_len)){
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


      if(remove_hdr == 0){

         /* skip our fake Received: line */

         hdr_ptr = strstr(buf, "Received: ");
         if(hdr_ptr){
            hdr_ptr += 10;
            if(is_valid_clapf_id(hdr_ptr)){
               continue;
            }
         }


         /* fix subject line if we have to */

         if(strncmp(buf, "Subject:", 8) == 0){
            has_subject = 1;

            if(put_subject_spam_prefix == 1 && !strstr(buf, cfg->spam_subject_prefix) ){
               strncat(headerbuf, "Subject:", sizeof(headerbuf)-1);
               strncat(headerbuf, cfg->spam_subject_prefix, sizeof(headerbuf)-1);
               strncat(headerbuf, buf+8, sizeof(headerbuf)-1);
               sent_subject_spam_prefix = 1;
            }
            else if(put_subject_possible_spam_prefix == 1 && !strstr(buf, cfg->possible_spam_subject_prefix) ){
               strncat(headerbuf, "Subject:", sizeof(headerbuf)-1);
               strncat(headerbuf, cfg->possible_spam_subject_prefix, sizeof(headerbuf)-1);
               strncat(headerbuf, buf+8, sizeof(headerbuf)-1);
               sent_subject_spam_prefix = 1;
            }
            else strncat(headerbuf, buf, sizeof(headerbuf)-1);
         }

         /* or just append the header line */

         else strncat(headerbuf, buf, sizeof(headerbuf)-1);

         strncat(headerbuf, "\n", sizeof(headerbuf)-1);
      }
      else remove_hdr = 0;

   } while(p && p < q);


   if(has_subject == 0){
      if((put_subject_spam_prefix == 1 || put_subject_possible_spam_prefix == 1) && sent_subject_spam_prefix == 0){
         strncat(headerbuf, "Subject: ", sizeof(headerbuf)-1);
         strncat(headerbuf, cfg->spam_subject_prefix, sizeof(headerbuf)-1);
         strncat(headerbuf, "\r\n", sizeof(headerbuf)-1);
      }
      else strncat(headerbuf, "Subject:\r\n", sizeof(headerbuf)-1);

   }

   /* append the spaminessbuf to the end of the header */
   if(spaminessbuf) strncat(headerbuf, spaminessbuf, sizeof(headerbuf)-1);

   if(is_header == 1){
      strncat(headerbuf, "\r\n\r\n.\r\n", sizeof(headerbuf)-1);
      i = n;
   }

   send(sd, headerbuf, strlen(headerbuf), 0);

   return i;
}



int smtp_chat(int sd, char *cmd, int ncmd, char *expect, char *buf, char *ttmpfile, int verbosity){
   int ok=1, n;
   char *p, puf[SMALLBUFSIZE];

   send(sd, cmd, strlen(cmd), 0);
   if(verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", ttmpfile, cmd);

READ:
   recvtimeout(sd, buf, MAXBUFSIZE, TIMEOUT);
   if(verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got in injecting: %s", ttmpfile, buf);

   n = 0;
   p = buf;

   while((p = split_str(p, "\r\n", puf, sizeof(puf)-1))){
      n++;
      if(strncmp(puf, "250", 3) && strncmp(puf, expect, 3)) ok = 0;
   }

   /* if we got less answer, then read on */

   if(n < ncmd){ ncmd -= n;  goto READ; }

   if(ok == 0){
      send(sd, "QUIT\r\n", 6, 0);
      if(verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: QUIT", ttmpfile);
      close(sd);
      syslog(LOG_PRIORITY, "%s: %s failed (%s)", ttmpfile, cmd, buf);
      return 1;
   }

   return 0;
}


/*
 * inject mail back to postfix
 */

int inject_mail(struct session_data *sdata, int msg, char *spaminessbuf, char *buf, struct __config *cfg){
   int i, k, n, psd, has_pipelining=0, ncmd=0;
   char puf[MAXBUFSIZE], bigbuf[MAX_MAIL_HEADER_SIZE], oursigno[SMALLBUFSIZE];
   char tmpbuf[SMALLBUFSIZE];
   struct in_addr addr;
   struct sockaddr_in postfix_addr;
   int fd;
   int num_of_reads=0;
   int put_subject_spam_prefix=0, put_subject_possible_spam_prefix=0;

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: trying to inject back", sdata->ttmpfile);


   memset(oursigno, 0, sizeof(oursigno));
   if(strlen(cfg->our_signo) > 3)
      snprintf(oursigno, sizeof(oursigno)-1, "%s\r\n", cfg->our_signo);


   if(strlen(cfg->smtp_addr) < 3 || cfg->smtp_port <= 0){
      syslog(LOG_PRIORITY, "%s: error: invalid smtp address (%s) or port (%d)", sdata->ttmpfile, cfg->smtp_addr, cfg->smtp_port);
      return ERR_INJECT;
   }

   if((psd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      syslog(LOG_PRIORITY, "%s: error: create socket", sdata->ttmpfile);
      return ERR_INJECT;
   }

   postfix_addr.sin_family = AF_INET;
   postfix_addr.sin_port = htons(cfg->smtp_port);
   inet_aton(cfg->smtp_addr, &addr);
   postfix_addr.sin_addr.s_addr = addr.s_addr;
   bzero(&(postfix_addr.sin_zero), 8);

   if(connect(psd, (struct sockaddr *)&postfix_addr, sizeof(struct sockaddr)) == -1){
      syslog(LOG_PRIORITY, "%s: error: connect to %s %d", sdata->ttmpfile, cfg->smtp_addr, cfg->smtp_port);
      return ERR_INJECT;
   }

   n = recvtimeout(psd, buf, MAXBUFSIZE, TIMEOUT);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got in injecting: %s", sdata->ttmpfile, buf);

   /* 220 banner */

   if(strncmp(buf, "220", 3)){
      send(psd, "QUIT\r\n", 6, 0);
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, SMTP_CMD_QUIT);
      close(psd);
      syslog(LOG_PRIORITY, "%s: missing 220 banner (%s)", sdata->ttmpfile, buf);
      return ERR_INJECT;
   }


   /* HELO/EHLO */

   snprintf(buf, MAXBUFSIZE-1, "EHLO %s\r\n", cfg->hostid);
   if(smtp_chat(psd, buf, 1, "250", &puf[0], sdata->ttmpfile, cfg->verbosity)) return ERR_INJECT;

   if(strstr(puf, "PIPELINING")) has_pipelining = 1;

   ncmd = 0;
   memset(buf, 0, MAXBUFSIZE);


   /*
    * assemble a pipelined command combo (MAIL
    * FROM, RCPT TO and DATA) if appropriate
    */

   /* MAIL FROM */

   strncat(buf, sdata->mailfrom, MAXBUFSIZE-1); ncmd++;
   if(!has_pipelining){ if(smtp_chat(psd, buf, 1, "250", &puf[0], sdata->ttmpfile, cfg->verbosity)) return ERR_INJECT; }


   /* RCPT TO */

   if(cfg->server_mode == SMTP_MODE){ i = 0; k = sdata->num_of_rcpt_to; } else { i = msg; k = msg+1; }

   for(; i<k; i++){

      if(!has_pipelining){ if(smtp_chat(psd, sdata->rcptto[i], 1, "250", &puf[0], sdata->ttmpfile, cfg->verbosity)) return ERR_INJECT; }
      else {
         snprintf(tmpbuf, sizeof(tmpbuf)-1, "RCPT TO: <%s>\r\n", sdata->rcptto[i]);

         if(strlen(buf) > MAXBUFSIZE/2){
            if(smtp_chat(psd, buf, ncmd, "250", &puf[0], sdata->ttmpfile, cfg->verbosity)) return ERR_INJECT;
            snprintf(buf, MAXBUFSIZE-1, "%s", tmpbuf);
            ncmd = 0;
         }
         else {
            strncat(buf, tmpbuf, MAXBUFSIZE-1);
            ncmd++;
         }
      }
   }


   /* DATA */

   strncat(buf, "DATA\r\n", MAXBUFSIZE-1);
   ncmd++;

   if(!has_pipelining){ snprintf(buf, MAXBUFSIZE-1, "DATA\r\n"); ncmd = 1; }

   /* send the rest of the combo command */
   if(smtp_chat(psd, buf, ncmd, "354", &puf[0], sdata->ttmpfile, cfg->verbosity)) return ERR_INJECT;


   /* send data */

      /* read and send stored mail */

         fd = open(sdata->ttmpfile, O_RDONLY);
         if(fd == -1)
            return ERR_INJECT;

         /* is this message spam and do we have to put [sp@m] prefix to the Subject: line? */

         if(spaminessbuf && strlen(cfg->spam_subject_prefix) > 2 && strstr(spaminessbuf, cfg->clapf_spam_header_field))
            put_subject_spam_prefix = 1;

         if(sdata->spaminess < cfg->spam_overall_limit && sdata->spaminess > cfg->possible_spam_limit)
            put_subject_possible_spam_prefix = 1;

         /*
          * the header_size_limit variable of postfix (see the output of postconf)
          * limits the length of the message header. So we read a big one and it
          * will contain the whole header. Now we can rewrite the necessary header
          * lines.
          */

         while((n = read(fd, bigbuf, sizeof(bigbuf))) > 0){
            num_of_reads++;

            /* the first read should contain all the header lines */

            if(num_of_reads == 1){

               /* send the header lines first */
               i = send_headers(psd, bigbuf, n, spaminessbuf, put_subject_spam_prefix, put_subject_possible_spam_prefix, sdata, cfg);

               /* then the rest of the first read */
               send(psd, &bigbuf[i], n-i, 0);

            } /* end of first read */
            else
               send(psd, bigbuf, n, 0);

         }

         close(fd);

         send(psd, "\r\n.\r\n", 5, 0);


   n = recvtimeout(psd, buf, MAXBUFSIZE, TIMEOUT);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got in injecting: %s", sdata->ttmpfile, buf);

   buf[n-2] = '\0';

   /* if our message was not accepted */
   if(strncmp(buf, "250", 3))
      syslog(LOG_PRIORITY, "%s: injecting failed (%s)", sdata->ttmpfile, buf);


   send(psd, "QUIT\r\n", 6, 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent in injecting: %s", sdata->ttmpfile, SMTP_CMD_QUIT);

   close(psd);

   if(strncmp(buf, "250", 3) == 0)
      return OK;
   else if(strncmp(buf, "550", 3) == 0)
      return ERR_REJECT;
   else
      return ERR_INJECT;
}
