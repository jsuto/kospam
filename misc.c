/*
 * misc.c, 2008.01.12, SJ
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


/*
 * count the invalid characters (ie. garbage on your display) in the buffer
 */

int count_invalid_junk(unsigned char *p){
   int i=0;

   for(; *p; p++){
      if(invalid_junk_characters[(unsigned int)*p] != ' '){
         /* 2006.05.09, SJ */
         *p = JUNK_REPLACEMENT_CHAR;

         i++;
      }
   }

   return i;
}


/*
 * invalid junk chars in the form of =xx 
 */

int count_invalid_hexa_stuff(unsigned char *p){
   unsigned char c;
   int i, c_hex_shit=0;
   char ch[3];

   for(i=0; i<strlen((char*)p); i++){
      if(p[i] == '=' && isxdigit(p[i+1]) && isxdigit(p[i+2])){
         ch[0] = '0';
         ch[1] = p[i+1];
         ch[2] = p[i+2];
         c = strtol(ch, NULL, 16) % 255;

         if(invalid_junk_characters[c] != ' '){
            c_hex_shit++;
         }

         i += 2;
      }
   }


   return c_hex_shit;
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
 * case insensitive strstr
 */

char *str_case_str(char *s, char *what){
   int i, k, slen=strlen(s), whatlen = strlen(what), yes;

   if(s == NULL || what == NULL)
      return NULL;

   if(slen < whatlen)
      return NULL;

   for(i=0; i<slen-whatlen; i++){
      yes = 1;
      for(k=0; k<whatlen; k++){
         if(s[i+k] != what[k] && s[i+k] != tolower(what[k]) && s[i+k] != toupper(what[k])){
            yes = 0;
            break;
         }
      }
      if(yes == 1)
         return &s[i];
   }

   return NULL;
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
 * create socket for Qcache
 */

int qcache_socket(char *qcache_addr, int qcache_port, char *qcache_socket){
   int sd;
   char buf[SMALLBUFSIZE];

   #ifdef HAVE_TCP
      struct sockaddr_in their_addr;
      struct in_addr addr;

      if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
         syslog(LOG_PRIORITY, "cannot create socket to Qcache");
         return -1;
      }

      their_addr.sin_family = AF_INET;
      their_addr.sin_port = htons(qcache_port);
      inet_aton(qcache_addr, &addr);
      their_addr.sin_addr.s_addr = addr.s_addr;
      memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);

      if(connect(sd, (struct sockaddr *)&their_addr, sizeof their_addr) == -1){
         close(sd);
         syslog(LOG_PRIORITY, "cannot connect to Qcache");
         return -1;
      }

   #else
      struct sockaddr_un server;

      if((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
         syslog(LOG_PRIORITY, "cannot create socket to Qcache");
         return -1;
      }

      strcpy(server.sun_path, qcache_socket);
      server.sun_family = AF_UNIX;

      if(connect(sd, (struct sockaddr *)&server, strlen(server.sun_path) + sizeof (server.sun_family)) == -1){
         close(sd);
         syslog(LOG_PRIORITY, "cannot connect to Qcache");
         return -1;
      }
   #endif

   /* read the Qcache banner */

   memset(buf, 0, SMALLBUFSIZE);
   recv(sd, buf, SMALLBUFSIZE-1, 0);

   return sd;
}


/*
 * write delivery info to file
 */

void write_delivery_info(char *tmpfile, char *dir, char *mailfrom, char rcptto[MAX_RCPT_TO][MAXBUFSIZE], int num_of_rcpt_to){
   int i;
   char qdelivery[SMALLBUFSIZE];
   FILE *f;

   snprintf(qdelivery, SMALLBUFSIZE-1, "%s/%s.d", dir, tmpfile);

   f = fopen(qdelivery, "w+");
   if(f){
      fprintf(f, mailfrom);
         for(i=0; i<num_of_rcpt_to; i++)
            fprintf(f, rcptto[i]);

      fclose(f);
   }
   else
      syslog(LOG_PRIORITY, "%s: failed writing delivery info to %s", tmpfile, qdelivery);
}


/*
 * move message to quarantine
 */

int move_message_to_quarantine(char *tmpfile, char *quarantine_dir, char *mailfrom, char rcptto[MAX_RCPT_TO][MAXBUFSIZE], int num_of_rcpt_to){
   char qfile[QUARANTINELEN];

   snprintf(qfile, QUARANTINELEN-1, "%s/%s", quarantine_dir, tmpfile);

   /* put down delivery info, 2006.01.19, SJ */

   if(rename(tmpfile, qfile) == 0){
      syslog(LOG_PRIORITY, "%s saved as %s", tmpfile, qfile);
      chmod(qfile, 0644);

      write_delivery_info(tmpfile, quarantine_dir, mailfrom, rcptto, num_of_rcpt_to);

      return OK;
   }
   else {
      syslog(LOG_PRIORITY, "failed to put %s into quarantine: %s", tmpfile, qfile);
      return ERR_MOVED;
   }

}
