/*
 * tokenizer.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <clapf.h>


int generate_tokens_from_string(struct __state *state, char *s, char *label){
   int i=0, len, result, n=0;
   char *p, puf[SMALLBUFSIZE], tmpbuf[SMALLBUFSIZE], chain[SMALLBUFSIZE];

   p = s;
   do {
      memset(puf, 0, sizeof(puf));
      p = split(p, ' ', puf, sizeof(puf)-1, &result);

      len = strlen(puf) ;

      if(len > 0){
         if(len == 1 && puf[0] == '-') continue;

         if(i > 0){
            snprintf(chain, sizeof(chain)-1, "%s+%s%s", tmpbuf, label, puf);
            addnode(state->token_hash, chain, DEFAULT_SPAMICITY, 0);
            n++;
         }

         snprintf(tmpbuf, sizeof(tmpbuf)-1, "%s%s", label, puf);
         if(len > 2){
            addnode(state->token_hash, tmpbuf, DEFAULT_SPAMICITY, 0);
            n++;
         }

         i++;
      }
   } while(p);

   return n;
}


void tokenize(char *buf, struct __state *state, struct session_data *sdata, struct __config *cfg){
   int x, result;
   unsigned int len;
   char *p, *q, u[SMALLBUFSIZE], puf[SMALLBUFSIZE];

   /*
    * we are interested in only the lines starting with "Received: from...".
    * The rest after the " by ..." part has no information to us
    */

   if(state->message_state == MSG_RECEIVED){
      if(strncmp(buf, "Received:", 9) == 0){
         p = strstr(buf, " by ");
         if(p) *p = '\0';
      }
      else return;
   }


   translate_line((unsigned char*)buf, state);

   reassemble_token(buf);


   if(state->is_header == 1) p = strchr(buf, ' ');
   else p = buf;

   //printf("a: %d/%d/%d/%d/q=%d %s\n", state->is_1st_header, state->is_header, state->message_rfc822, state->message_state, state->qp, buf);

   do {
      memset(puf, 0, sizeof(puf));
      p = split(p, ' ', puf, sizeof(puf)-1, &result);

      if(puf[0] == '\0') continue;

      degenerate_token((unsigned char*)puf);

      if(puf[0] == '\0') continue;

      strncat(puf, " ", sizeof(puf)-1);
      len = strlen(puf);

      if(strncasecmp(puf, "http://", 7) == 0 || strncasecmp(puf, "https://", 8) == 0) { fix_URL(puf); len = strlen(puf); }

      if(state->is_header == 0 && strncmp(puf, "URL*", 4) && (puf[0] == ' ' || (len > MAX_WORD_LEN && cfg->enable_cjk == 0) || is_hex_number(puf)) ) continue;

      if(state->message_state == MSG_BODY && len >= (unsigned int)cfg->min_word_len && state->bodylen < BIGBUFSIZE-len-1){
         memcpy(&(state->b_body[state->bodylen]), puf, len);
         state->bodylen += len;

         if(!strncmp(puf, "URL*", 4)){
            addnode(state->url, puf, DEFAULT_SPAMICITY, 0);

            get_tld_from_name(puf);
            addnode(state->token_hash, puf, DEFAULT_SPAMICITY, 0);
            state->n_token++;
         }

      }

      else if(state->message_state == MSG_RECEIVED){
         puf[len-1] = '\0';
         x = puf[len-2];

         /*
          * skip Received line token, if
          *    - no punctuation (eg. by, from, esmtp, ...)
          *    - not "unknown"
          *    - it's on the skipped_received_hosts or skipped_received_ips list
          *    - ends with a number and not a valid IP-address (8.14.3, 6.0.3790.211, ...)
          */

         if((!strchr(puf, '.') && strcmp(puf, "unknown")) || ( (x >= 0x30 && x <= 0x39) && (is_dotted_ipv4_address(puf) == 0 || count_character_in_buffer(puf, '.') != 3) ) ){
            continue;
         }


         /*
          * fill state.ip and state.hostname fields _after_
          * eliminated all entries matched by skipped_received_ips,
          * and skipped_received_hosts.
          * These entries hold the name and address of the host
          * which hands this email to us.
          */

         if(sdata->ipcnt <= 1){
            if(is_dotted_ipv4_address(puf) == 1){
               snprintf(sdata->ip, SMALLBUFSIZE-1, "%s", puf);
               if(is_item_on_list(puf, cfg->skipped_received_ips, "127.,10.,192.168.,172.16.") == 0) sdata->ipcnt = 1;
            }
            else {
               snprintf(sdata->hostname, SMALLBUFSIZE-1, "%s", puf);
            }
         }

         continue;
      }

      else if(state->message_state == MSG_FROM && state->is_1st_header == 1 && strlen(state->b_from) < SMALLBUFSIZE-len-1){
         memcpy(&(state->b_from[strlen(state->b_from)]), puf, len);

         if(does_it_seem_like_an_email_address(puf) == 1 && state->b_from_domain[0] == '\0' && len > 5){
            q = strchr(puf, '@');
            if(q && strlen(q) > 5){
               memcpy(&(state->b_from_domain), q+1, strlen(q+1)-1);
            }
         }
      }

      else if((state->message_state == MSG_TO || state->message_state == MSG_CC || state->message_state == MSG_RECIPIENT) && state->is_1st_header == 1){
         if(strchr(puf, '@') ) continue;
      }


   } while(p);


   if(sdata->ipcnt == 1){
      sdata->ipcnt = 2;

      snprintf(puf, sizeof(puf)-1, "HEADER*%s", sdata->hostname);
      addnode(state->token_hash, puf, DEFAULT_SPAMICITY, 0);
      state->n_token++;

      snprintf(u, sizeof(u)-1, "%s", sdata->hostname);
      if(strlen(u) > MAX_WORD_LEN) fix_FQDN(u);

      snprintf(puf, sizeof(puf)-1, "HEADER*%s", u);
      addnode(state->token_hash, puf, DEFAULT_SPAMICITY, 0);
      state->n_token++;

      snprintf(puf, sizeof(puf)-1, "HEADER*%s", sdata->ip);
      addnode(state->token_hash, puf, DEFAULT_SPAMICITY, 0);
      state->n_token++;

   }


}

