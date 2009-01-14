/*
 * misc.c, 2009.01.14, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <ctype.h>
#include "misc.h"
#include "trans.h"
#include "ijc.h"
#include "smtpcodes.h"
#include "errmsg.h"
#include "config.h"


/*
 * fatal functions for quitting
 */

void _fatal(char *s){
   syslog(LOG_PRIORITY, "%s\n", s);
   exit(1);
}

void __fatal(char *s){
   fprintf(stderr, "%s\n", s);
   exit(1);
}

/*
 * calculate the difference betwwen two timevals in [usec]
 */

long tvdiff(struct timeval a, struct timeval b){
   double res;

   res = (a.tv_sec * 1000000 + a.tv_usec) - (b.tv_sec * 1000000 + b.tv_usec);
   return (long) res;
}


/*
 * translate apostrophes in buffer
 */

void pre_translate(char *p){
   for(; *p; p++){
      if(*p == '"' || *p == '\'')
        *p = ' ';
   }
}

/*
 * translate buffer and preserve urls
 */

int translate(unsigned char *p, int qp){
   int url=0, clear=0;
   unsigned char *q=NULL, *P=p;

   for(; *p; p++){

   #ifndef HAVE_CASE
      *p = tolower(*p);
   #endif

      /* save position of '=', 2006.01.05, SJ */

      if(qp == 1 && *p == '='){
         q = p;
      }


      if(strncasecmp((char *)p, "http://", 7) == 0){ p += 7; url = 1; continue; }
      if(strncasecmp((char *)p, "https://", 8) == 0){ p += 8; url = 1; continue; }

      if(url == 1 && (*p == '?' || *p == '/'))
         clear = 1;

      if(isspace(*p))
          clear = 0;

      if(clear == 1){
         /* save the url as it is, 2007.06.21, SJ */
         *p = ' ';
      }
      else {
         *p = translated_characters[(unsigned int)*p];
      }
   }

   /* restore the soft break in quoted-printable parts, 2006.01.05, SJ */

   if(qp == 1 && q && (q > P + strlen((char*)P) - 3))
     *q = '=';


   if(url == 1)
      return 1;
   else
      return 0;
}

int translate2(unsigned char *p, int qp){
   int url=0, clear=0;
   unsigned char *q=NULL, *P=p;

   for(; *p; p++){

      /* save position of '=', 2006.01.05, SJ */

      if(qp == 1 && *p == '='){
         q = p;
      }


      if(strncasecmp((char *)p, "http://", 7) == 0){ p += 7; url = 1; continue; }
      if(strncasecmp((char *)p, "https://", 8) == 0){ p += 8; url = 1; continue; }

      if(url == 1 && (*p == '?' || *p == '/')) clear = 1;

      /* clear the rest of the url, eg. http://aaa.fu/ahah.cgi?aa=bb => http://aaa.fu */

      if(isspace(*p)) clear = 0;
      if(clear == 1){
         *p = ' ';
         continue;
      }

      if(delimiter_characters[(unsigned int)*p] != ' ')
         *p = ' ';
      else {
      #ifndef HAVE_CASE
         *p = tolower(*p);
      #endif
      }
   }

   /* restore the soft break in quoted-printable parts, 2006.01.05, SJ */

   if(qp == 1 && q && (q > P + strlen((char*)P) - 3))
     *q = '=';

   return 0;
}


/*
 * reassemble 'V i a g r a' to 'Viagra'
 */

void reassemble_token(char *p){
   int i, k=0;

   for(i=0; i<strlen(p); i++){

      if(isprint(*(p+i)) && *(p+i+1) == ' ' && isprint(*(p+i+2)) && *(p+i+3) == ' ' && isprint(*(p+i+4)) && *(p+i+5) == ' '){
         p[k] = *(p+i); k++;
         p[k] = *(p+i+2); k++;
         p[k] = *(p+i+4); k++;

         //fprintf(stderr, "%c*%c*", *(p+i), *(p+i+2));
         i += 5;
      }
      else {
         p[k] = *(p+i);
         k++;
         //fprintf(stderr, "%c", *(p+i));
      }
   }

   p[k] = '\0';

}


/*
 * count the invalid characters (ie. garbage on your display) in the buffer
 */

int count_invalid_junk(char *p, int replace_junk){
   int i=0;

   for(; *p; p++){
      if(invalid_junk_characters[(unsigned char)*p] == *p){
         i++;
         if(replace_junk == 1) *p = JUNK_REPLACEMENT_CHAR;
      }
   }

   return i;
}


/*
 * has the token got strange punctuations?
 */

int is_odd_punctuations(char *p){
   for(; *p; p++){
      if(ispunct(*p) && ispunct(*(p+1)) && ispunct(*(p+2)))
         return 1;
   }

   return 0;
}


/*
 * is this a numeric string?
 */

int is_number(char *p){
   for(; *p; p++){
      if((*p < 0x30 || *p > 0x39) && *p != '-')
         return 0;
   }

   return 1;
}


/*
 * is this a hexadecimal numeric string?
 */

int is_hex_number(char *p){
   for(; *p; p++){
      if(!(*p == '-' || (*p >= 0x30 && *p <= 0x39) || (*p >= 0x41 && *p <= 0x47) || (*p >= 0x61 && *p <= 0x67)) )
         return 0;
   }

   return 1;
}


/*
 * search something in a buffer
 */

int search_in_buf(char *s, int len1, char *what, int len2){
   int i, k, r;

   for(i=0; i<len1; i++){
      r = 0;

      for(k=0; k<len2; k++){
         if(*(s+i+k) == *(what+k))
            r++;
      }

      if(r == len2)
         return 1;
   }

   return 0;
}


/*
 * count a character in buffer
 */

int count_char_in_buffer(char *p, char c){
   int i=0;

   for(; *p; p++){
      if(*p == c)
         i++;
   }

   return i;
}


/*
 * degenerate a token
 */


void degenerate(unsigned char *p){
   int i=1, d=0, dp=0;
   unsigned char *s;

   /* quit if this the string does not end with a punctuation character */

   if(!ispunct(*(p+strlen((char *)p)-1)))
      return;

   s = p;

   for(; *p; p++){
      if(ispunct(*p)){
         d = i;

         if(!ispunct(*(p-1)))
            dp = d;
      }
      else
         d = dp = i;

      i++;
   }

   *(s+dp) = '\0';
}


/*
 * fix a long URL
 */

void fix_url(char *url){
   char *p, *q, m[MAX_TOKEN_LEN], fixed_url[MAXBUFSIZE];
   int i, dots=0;
   struct in_addr addr;

   /* chop trailing dot */

   if(url[strlen(url)-1] == '.')
      url[strlen(url)-1] = '\0';

   memset(fixed_url, 0, MAXBUFSIZE);

   if((strncasecmp(url, "http://", 7) == 0 || strncasecmp(url, "https://", 8) == 0) ){
      p = url;

      if(strncasecmp(p, "http://", 7) == 0) p += 7;
      if(strncasecmp(p, "https://", 8) == 0) p += 8;

      /* skip anything after the host part, 2006.12.11, SJ */
      q = strchr(p, '/');
      if(q)
         *q = '\0';

      /*
         http://www.ajandekkaracsonyra.hu/email.php?page=email&cmd=unsubscribe&email=yy@xxxx.kom is
         chopped to www.ajandekkaracsonyra.hu at this point, 2006.12.15, SJ
       */

      dots = count_char_in_buffer(p, '.');
      if(dots < 1)
         return;

      strncpy(fixed_url, "URL*", MAXBUFSIZE-1);

      /* is it a numeric IP-address? */

      if(inet_aton(p, &addr)){
         addr.s_addr = ntohl(addr.s_addr);
         strncat(fixed_url, inet_ntoa(addr), MAXBUFSIZE-1);
         strcpy(url, fixed_url);
      }
      else {
         //dots = count_char_in_buffer(url, '.');
         for(i=0; i<=dots; i++){
            q = split(p, '.', m, MAX_TOKEN_LEN-1);
            if(i>dots-2){
               strncat(fixed_url, m, MAXBUFSIZE-1);
               if(i < dots)
                  strncat(fixed_url, ".", MAXBUFSIZE-1);
            }
            p = q;
         }

         /* if it does not contain a single dot, the rest of the URL may be
            in the next line or it is a fake one, anyway skip, 2006.04.06, SJ
          */

         if(count_char_in_buffer(fixed_url, '.') != 1)
            memset(url, 0, MAXBUFSIZE);
         else {
            for(i=4; i<strlen(fixed_url); i++)
               fixed_url[i] = tolower(fixed_url[i]);

            strcpy(url, fixed_url);
         }
      }
   }

}


/*
 * fix a long FQDN
 */

void fix_fqdn(char *fqdn){
   char *p, *q, m[MAX_TOKEN_LEN], fixed_fqdn[MAXBUFSIZE];
   int i, dots=0;

   /* chop trailing dot */

   if(fqdn[strlen(fqdn)-1] == '.')
      fqdn[strlen(fqdn)-1] = '\0';

   memset(fixed_fqdn, 0, MAXBUFSIZE);

   p = fqdn;

   dots = count_char_in_buffer(p, '.');
   if(dots < 1)
      return;

   for(i=0; i<=dots; i++){
      q = split(p, '.', m, MAX_TOKEN_LEN-1);
      if(i>dots-2){
         strncat(fixed_fqdn, m, MAXBUFSIZE-1);
         if(i < dots)
             strncat(fixed_fqdn, ".", MAXBUFSIZE-1);
      }
      p = q;
   }

   strcpy(fqdn, fixed_fqdn);
}


/*
 * split a string by a character as delimiter
 */

char *split(char *row, int ch, char *s, int size){
   char *r;
   int len;

   if(row == NULL)
      return NULL;

   r = strchr(row, ch);
   if(r == NULL){
      len = strlen(row);
      if(len > size)
         len = size;
   }
   else {
      len = strlen(row) - strlen(r);
      if(len > size)
         len = size;

      r++;
   }

   if(s != NULL){
      strncpy(s, row, len);
      s[len] = '\0';
   }

   return r;
}


/*
 * split a string by a string as delimiter
 */

char *split_str(char *row, char *what, char *s, int size){
   char *r;
   int len;

   if(row == NULL)
      return NULL;

   r = strstr(row, what);
   if(r == NULL){
      len = strlen(row);
      if(len > size)
         len = size;
   }
   else {
      len = strlen(row) - strlen(r);
      if(len > size)
         len = size;

      r += strlen(what);
   }

   if(s != NULL){
      strncpy(s, row, len);
      s[len] = '\0';
   }

   return r;
}


/*
 * APHash function
 * http://www.partow.net/programming/hashfunctions/#APHashFunction
 */

unsigned long long APHash(char *p){
   unsigned long long hash = 0;
   int i=0;

   for(; *p; p++){
      hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ (*p) ^ (hash >> 3)) :
                               (~((hash << 11) ^ (*p) ^ (hash >> 5)));
      i++;
   }

   return hash % MAX_KEY_VAL;
}


/*
 * trim trailing CR-LF
 */

void trim(char *what){
   char *p;

   p = strrchr(what, '\n');
   if(p) *p = '\0';

   p = strrchr(what, '\r');
   if(p) *p = '\0';

}


/*
 * replace a character with a different one
 */

void replace(char *p, int what, int with){
   for(; *p; p++){
      if(*p == what) *p = with;
   }
}


/*
 * extract email
 */

int extract_email(char *rawmail, char *email){
   char *p;

   p = strchr(rawmail, '<');
   if(p){
      snprintf(email, SMALLBUFSIZE-1, "%s", p+1);
      p = strchr(email, '>');
      if(p){
         *p = '\0';
         return 1;
      }
   }

   return 0;
}


/*
 * read random data from entropy pool
 */

int readfrompool(int fd, void *_s, size_t n){
   char *s = _s;
   ssize_t res, pos = 0;

   while(n > pos){
      res = read(fd, s + pos, n - pos);
      switch(res){
         case  -1: continue;
         case   0: return res;
         default : pos += res;
      }
   }
   return pos;
}

/*
 * reading from pool
 */

int get_random_bytes(unsigned char *buf, int len){
   int random_pool;
   unsigned char a, b;

   /* the first 4 bytes are the timestamp, 2007.12.13, SJ */

   time((time_t*)&buf[0]);

   a = buf[0];
   b = buf[3];

   buf[0] = b;
   buf[3] = a;

   a = buf[1];
   b = buf[2];

   buf[1] = b;
   buf[2] = a;

   random_pool = open(RANDOM_POOL, O_RDONLY);
   if(random_pool == -1)
       return 0;

   if(readfrompool(random_pool, buf+4, len-4) != len-4){
       syslog(LOG_PRIORITY, "%s: %s", ERR_CANNOT_READ_FROM_POOL, RANDOM_POOL);
       close(random_pool);
       return 0;
   }
   
   close(random_pool);
   return 1;
}

/*
 * recv() with timeout
 */

int recvtimeout(int s, char *buf, int len, int timeout){
    fd_set fds;
    int n;
    struct timeval tv;

    memset(buf, 0, MAXBUFSIZE);

    FD_ZERO(&fds);
    FD_SET(s, &fds);

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = TIMEOUT_USEC;

    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    return recv(s, buf, len, 0);
}


int recvtimeout2(int s, unsigned char *buf, int len, int timeout){
    fd_set fds;
    int n;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(s, &fds);

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = TIMEOUT_USEC;

    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    return recv(s, buf, len, 0);
}


/*
 * create a random ID
 */

int make_rnd_string(char *res){
   int i;
   unsigned char buf[RND_STR_LEN/2];

   get_random_bytes(buf, RND_STR_LEN/2);

   for(i=0; i < (RND_STR_LEN/2)-1; i++){
      sprintf(res, "%02x", buf[i]);
      res += 2;
   }

   return 1;
}


/*
 * check if it's a valid ID
 */

int is_valid_id(char *p){

   if(strlen(p) != 30)
      return 0;

   for(; *p; p++){
      /* 0-9: 0x30-0x39, a-f: 0x61-0x66 */

      if(! ((*p >= 0x30 && *p <= 0x39) || (*p >= 0x61 && *p <= 0x66) ) ){
         printf("%c*\n", *p);
         return 0;
      }
   }

   return 1;
}


/*
 * syslog ham/spam status per email addresses
 */

void log_ham_spam_per_email(char *tmpfile, char *email, int ham_or_spam){
   if(ham_or_spam == 0)
      syslog(LOG_PRIORITY, "%s: %s got HAM", tmpfile, email);
   else
      syslog(LOG_PRIORITY, "%s: %s got SPAM", tmpfile, email);
}


/*
 * extract messageid from forwarded messages
 */

int extract_id_from_message(char *messagefile, char *clapf_header_field, char *ID){
   int i=0, fd, len, train_mode=T_TOE;
   char *p, *q, *r, buf[8*MAXBUFSIZE], puf[SMALLBUFSIZE];

   memset(ID, 0, RND_STR_LEN+1);

   fd = open(messagefile, O_RDONLY);
   if(fd == -1) return -1;

   while((len = read(fd, buf, 8*MAXBUFSIZE)) > 0){
      /* data should be here in the first read */

      p = buf;
      do {
         p = split(p, '\n', puf, SMALLBUFSIZE-1);

         q = strstr(puf, clapf_header_field);
         if(q){
            trim(puf);
            r = strchr(puf, ' ');
            if(r){
               r++;
               if(is_valid_id(r)){
                  i++;
                  if(i <= 1){
                     snprintf(ID, RND_STR_LEN, "%s", r);
                  }
               }
            }
         }

         if(strlen(ID) > 2 && strncmp(puf, clapf_header_field, strlen(clapf_header_field)) == 0){
            if(strncmp(puf + strlen(clapf_header_field), "TUM", 3) == 0)
               train_mode = T_TUM;
         }
      } while(p);

   }

   close(fd);

   return train_mode;
}


/*
 * write delivery info to file
 */

void write_delivery_info(struct session_data *sdata, char *dir){
   int i;
   FILE *f;

   snprintf(sdata->deliveryinfo, SMALLBUFSIZE-1, "%s/%s.d", dir, sdata->ttmpfile);

   f = fopen(sdata->deliveryinfo, "w+");
   if(f){
      fprintf(f, sdata->mailfrom);
         for(i=0; i<sdata->num_of_rcpt_to; i++)
            fprintf(f, sdata->rcptto[i]);

      fclose(f);
   }
   else
      syslog(LOG_PRIORITY, "%s: failed writing delivery info to %s", sdata->ttmpfile, sdata->deliveryinfo);
}


/*
 * move message to quarantine
 */

int move_message_to_quarantine(struct session_data *sdata, char *quarantine_dir){
   char qfile[QUARANTINELEN];

   snprintf(qfile, QUARANTINELEN-1, "%s/%s", quarantine_dir, sdata->ttmpfile);

   /* if we would use rename, we cannot unlink this file later, producing an
      error message to syslog */

   if(link(sdata->ttmpfile, qfile) == 0){
      syslog(LOG_PRIORITY, "%s saved as %s", sdata->ttmpfile, qfile);
      chmod(qfile, 0644);

      return OK;
   }
   else {
      syslog(LOG_PRIORITY, "failed to put %s into quarantine: %s", sdata->ttmpfile, qfile);
      return ERR_MOVED;
   }

}


/*
 * is the email address in our domains?
 */

int is_recipient_in_our_domains(char *rawmail, struct __config *cfg){
   char *p, email[SMALLBUFSIZE];

   if(extract_email(rawmail, email) == 0) return 0;

   p = strchr(email, '@');
   if(!p) return 0;

   //syslog(LOG_PRIORITY, "%s in %s", p, cfg->mydomains);

   if(strstr(cfg->mydomains, p)) return 1;

   return 0;
}


/*
 * resolve a hostname or dotted IPv4 address
 */

unsigned long resolve_host(char *h){
   struct hostent *host;
   struct in_addr addr;

   if(!h) return 0;

   if((addr.s_addr = inet_addr(h)) == -1){
       if((host = gethostbyname(h)) == NULL){
          return 0;
       }
       else return *(unsigned long*)host->h_addr;
   }
   else return addr.s_addr;
}


/*
 * whitelist check
 */

int whitelist_check(char *whitelist, char *tmpfile, char *email, struct __config *cfg){
   int r=0;
   char *p, *q, w[SMALLBUFSIZE];

   p = whitelist;

   if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "%s: whitelist: %s", tmpfile, whitelist);

   do {
      p = split(p, '\n', w, SMALLBUFSIZE-1);

      trim(w);

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: matching %s on %s", tmpfile, w, email);

      if(strlen(w) > 2){

         if(w[strlen(w)-1] == '$' && strlen(email) > strlen(w)){
            q = email + strlen(email) - strlen(w) + 1;
            if(strncasecmp(q, w, strlen(w)-1) == 0)
                r = 1;
         }
         else if(strcasestr(email, w))
            r = 1;

         if(r == 1){
            syslog(LOG_PRIORITY, "%s: found on whitelist (%s matches %s)", tmpfile, email, w);
            return r;
         }
      }

   } while(p);

   return r;
}


