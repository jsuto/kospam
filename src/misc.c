/*
 * misc.c, SJ
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
#include <sys/resource.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <clapf.h>


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


void disable_coredump() {
   struct rlimit rlim;

   rlim.rlim_cur = rlim.rlim_max = 0;
   setrlimit(RLIMIT_CORE, &rlim);
}


/*
 * search something in a buffer
 */

int search_string_in_buffer(char *s, int len1, char *what, int len2){
   int i, k, r;

   for(i=0; i<len1; i++){
      r = 0;

      for(k=0; k<len2; k++){
         if(*(s+i+k) == *(what+k))
            r++;
      }

      if(r == len2)
         return i;
   }

   return 0;
}


/*
 * count a character in buffer
 */

int count_character_in_buffer(char *p, char c){
   int i=0;

   for(; *p; p++){
      if(*p == c)
         i++;
   }

   return i;
}


void replace_character_in_buffer(char *p, char from, char to){
   int k=0;

   for(size_t i=0; i<strlen(p); i++){
      if(p[i] == from){
         if(to > 0){
            p[k] = to;
            k++;
         }
      }
      else {
         p[k] = p[i];
         k++;
      }

   }

   p[k] = '\0';
}


/*
 * split a string by a string as delimiter
 */

char *split_str(char *row, char *what, char *s, int size){
   char *r;
   int len;

   memset(s, 0, size);

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
 * trim trailing CR-LF
 */

int trim_buffer(char *s){
   int n=0;
   char *p;

   p = strrchr(s, '\n');
   if(p){
      *p = '\0';
      n++;
   }

   p = strrchr(s, '\r');
   if(p){
      *p = '\0';
      n++;
   }

   return n;
}


int extract_verp_address(char *email){
   char *p, *p1, *p2;
   char puf[SMALLBUFSIZE];

   // a VERP address is like archive+user=domain.com@myarchive.local

   if(!email) return 0;

   if(strlen(email) < 5) return 0;

   p1 = strchr(email, '+');
   if(p1){
      p2 = strchr(p1, '@');
      if(p2 && p2 > p1 + 2){
         if(strchr(p1+1, '=')){
            memset(puf, 0, sizeof(puf));

            memcpy(&puf[0], p1+1, p2-p1-1);
            p = strchr(puf, '=');
            if(p) *p = '@';
            strcpy(email, puf);
         }
      }

   }

   return 0;
}


int extractEmail(char *rawmail, char *email){
   char *p;

   memset(email, 0, SMALLBUFSIZE);

   p = strchr(rawmail, '<');
   if(p){
      snprintf(email, SMALLBUFSIZE-1, "%s", p+1);
      p = strchr(email, '>');
      if(p){
         *p = '\0';
         /*
          * we can't perform verp extraction here, because it breaks the rcpt to email used in the smtp injection, eg.
          *
          * <piler-user+confsub-215840341cf5090a-suto.janos=aaa.fu@list.acts.hu> -> confsub-215840341cf5090a-suto.janos@aaa.fu
          */
         return 1;
      }
   }

   return 0;
}


void make_random_string(char *buf, int buflen){
   int i, len;
   static char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

   len = strlen(alphanum);

   for(i=0; i<buflen; i++){
      *(buf+i) = alphanum[rand() % len];
   }

}


void create_id(char *id, unsigned char server_id){
   unsigned char buf[RND_STR_LEN/2];

   memset(id, 0, SMALLBUFSIZE);

   get_random_bytes(buf, sizeof(buf), server_id);

   for(size_t i=0; i < sizeof(buf); i++){
      sprintf(id, "%02x", buf[i]);
      id += 2;
   }
}


int is_valid_clapf_id(char *p){

   if(strlen(p) != 36)
      return 0;

   for(; *p; p++){
      /* 0-9: 0x30-0x39, a-f: 0x61-0x66 */

      if(! ((*p >= 0x30 && *p <= 0x39) || (*p >= 0x61 && *p <= 0x66) || *p == 0x0d) ){
         return 0;
      }
   }

   return 1;
}


/*
 * reading from pool
 */

int get_random_bytes(unsigned char *buf, int len, unsigned char server_id){
   int fd, ret=0;
   struct taia now;
   char nowpack[TAIA_PACK];

   /* the first 12 bytes are the taia timestamp */

   taia_now(&now);
   taia_pack(nowpack, &now);

   memcpy(buf, nowpack, 12);

   fd = open(RANDOM_POOL, O_RDONLY);
   if(fd == -1) return ret;

   *(buf + 12) = server_id;

   if(readFromEntropyPool(fd, buf+12+1, len-12-1) != len-12-1){
      syslog(LOG_PRIORITY, "%s: error: %s", ERR_CANNOT_READ_FROM_POOL, RANDOM_POOL);
   }

   close(fd);
   return ret;
}


/*
 * read random data from entropy pool
 */

int readFromEntropyPool(int fd, void *_s, size_t n){
   char *s = _s;
   size_t res, pos = 0;

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
 * recv() with timeout
 */

int recvtimeout(int s, char *buf, int len, int timeout){
    fd_set fds;
    int n;
    struct timeval tv;

    memset(buf, 0, len);

    FD_ZERO(&fds);
    FD_SET(s, &fds);

    tv.tv_sec = timeout;
    tv.tv_usec = TIMEOUT_USEC;

    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    return recv(s, buf, len, 0);
}


int write1(int sd, void *buf, int buflen, int use_ssl, SSL *ssl){
   int n;

   if(use_ssl == 1)
      n = SSL_write(ssl, buf, buflen);
   else
      n = send(sd, buf, buflen, 0);

   return n;
}


int ssl_want_retry(SSL *ssl, int ret, int timeout){
   int i;
   fd_set fds;
   struct timeval tv;
   int sock;

   // something went wrong.  I'll retry, die quietly, or complain
   i = SSL_get_error(ssl, ret);
   if(i == SSL_ERROR_NONE)
      return 1;

   tv.tv_sec = timeout/1000;
   tv.tv_usec = 0;
   FD_ZERO(&fds);

   switch(i){
      case SSL_ERROR_WANT_READ: // pause until the socket is readable
          sock = SSL_get_rfd(ssl);
          FD_SET(sock, &fds);
          i = select(sock+1, &fds, 0, 0, &tv);
          break;

      case SSL_ERROR_WANT_WRITE: // pause until the socket is writeable
         sock = SSL_get_wfd(ssl);
         FD_SET(sock, &fds);
         i = select(sock+1, 0, &fds, 0, &tv);
         break;

      case SSL_ERROR_ZERO_RETURN: // the sock closed, just return quietly
         i = 0;
         break;

      default: // ERROR - unexpected error code
         i = -1;
         break;
   };

   return i;
}


int ssl_read_timeout(SSL *ssl, void *buf, int len, int timeout){
   int i;

   while(1){
      i = SSL_read(ssl, (char*)buf, len);
      if(i > 0) break;
      i = ssl_want_retry(ssl, i, timeout);
      if(i <= 0) break;
   }

   return i;
}


int recvtimeoutssl(int s, char *buf, int len, int timeout, int use_ssl, SSL *ssl){

    memset(buf, 0, len);

    if(use_ssl == 1){
       return ssl_read_timeout(ssl, buf, len-1, timeout);
    }
    else {
       return recvtimeout(s, buf, len-1, timeout);
    }
}


void write_pid_file(char *pidfile){
   FILE *f;

   f = fopen(pidfile, "w");
   if(f){
      fprintf(f, "%d", (int)getpid());
      fclose(f);
   }
   else syslog(LOG_PRIORITY, "error: cannot write pidfile '%s'", pidfile);
}


int drop_privileges(struct passwd *pwd){

   if(pwd->pw_uid > 0 && pwd->pw_gid > 0){

      if(getgid() != pwd->pw_gid){
         if(setgid(pwd->pw_gid)) return -1;
      }

      if(getuid() != pwd->pw_uid){
         if(setuid(pwd->pw_uid)) return -1;
      }

   }

   return 0;
}


void init_session_data(struct session_data *sdata, struct __config *cfg){
   int i;


   sdata->fd = -1;

   create_id(&(sdata->ttmpfile[0]), cfg->server_id);
   unlink(sdata->ttmpfile);

   snprintf(sdata->filename, SMALLBUFSIZE-1, "%s", sdata->ttmpfile);

   memset(sdata->mailfrom, 0, SMALLBUFSIZE);

   memset(sdata->attachments, 0, SMALLBUFSIZE);

   memset(sdata->fromemail, 0, SMALLBUFSIZE);

   sdata->ipcnt = 0;
   memset(sdata->ip, 0, SMALLBUFSIZE);
   memset(sdata->hostname, 0, SMALLBUFSIZE);

   memset(sdata->whitelist, 0, MAXBUFSIZE);
   memset(sdata->blacklist, 0, MAXBUFSIZE);

   sdata->tot_len = 0;
   sdata->num_of_rcpt_to = 0;

   sdata->tre = '-';

   sdata->rav = AVIR_OK;

   sdata->__acquire = sdata->__parsed = sdata->__av = sdata->__user = sdata->__policy = sdata->__as = sdata->__minefield = 0;
   sdata->__training = sdata->__update = sdata->__store = sdata->__inject = 0;

   for(i=0; i<MAX_RCPT_TO; i++) memset(sdata->rcptto[i], 0, SMALLBUFSIZE);

   time(&(sdata->now));
   sdata->sent = sdata->now;

   sdata->sql_errno = 0;

   sdata->blackhole = 0;
   sdata->trapped_client = 0;

   sdata->spaminess = DEFAULT_SPAMICITY;
   sdata->need_signo_check = 0;

   sdata->uid = sdata->gid = 0;
   sdata->statistically_whitelisted = 0;
   sdata->training_request = 0;
   sdata->mynetwork = 0;

   sdata->uid = sdata->gid = 0;
   sdata->status = S_HAM;

   sdata->pid = getpid();

   //sdata->skip_id_check = 0;
   sdata->from_address_in_mydomain = 0;
   memset(sdata->name, 0, SMALLBUFSIZE);
   memset(sdata->domain, 0, SMALLBUFSIZE);
   memset(sdata->clapf_id, 0, SMALLBUFSIZE);
   //memset(sdata->subject, 0, SMALLBUFSIZE);
   //memset(sdata->rcpt_minefield, 0, MAX_RCPT_TO);
}


int read_from_stdin(struct session_data *sdata){
   int fd, rc=ERR, n;
   char buf[MAXBUFSIZE];

   fd = open(sdata->ttmpfile, O_CREAT|O_EXCL|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
   if(fd == -1){
      syslog(LOG_PRIORITY, "%s: error: open ttmpfile", sdata->ttmpfile);
      return rc;
   }

   while((n = read(0, buf, sizeof(buf))) > 0){
      sdata->tot_len += write(fd, buf, n);
   }

   if(fsync(fd)){
      syslog(LOG_PRIORITY, "%s: error: fsync()", sdata->ttmpfile);
   }
   else rc = OK;

   close(fd);

   return rc;
}


void strtolower(char *s){
   for(; *s; s++){
      if(*s >= 65 && *s <= 90) *s = tolower(*s);
   }
}


void *get_in_addr(struct sockaddr *sa){
   if(sa->sa_family == AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int can_i_write_current_directory(){
   int fd;
   char filename[SMALLBUFSIZE];

   snprintf(filename, sizeof(filename)-1, "__piler_%d", getpid());

   fd = open(filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP);
   if(fd == -1){
      return 0;
   }

   close(fd);
   unlink(filename);

   return 1;
}


int is_item_on_list(char *item, char *list, char *extralist){
   int result;
   char *p, *q, w[MAXBUFSIZE], my_list[MAXBUFSIZE];

   if(!item) return 0;

   if(strlen(list) + strlen(extralist) < 3) return 0;

   snprintf(my_list, sizeof(my_list)-1, "%s,%s", extralist, list);

   p = my_list;

   do {
      p = split(p, ',', w, sizeof(w)-1, &result);

      trim_buffer(w);

      if(strlen(w) > 2){

         if(w[strlen(w)-1] == '$'){
            q = item + strlen(item) - strlen(w) + 1;
            if(strncasecmp(q, w, strlen(w)-1) == 0){
               return 1;
            }
         }
         else if(strcasestr(item, w)){
            return 1;
         }

      }

   } while(p);

   return 0;
}


int is_list_on_string(char *s, char *list){
   int a;
   char *p, w[SMALLBUFSIZE];

   p = list;

   do {
      p = split(p, ',', w, sizeof(w)-1, &a);

      trim_buffer(w);

      if(strcasestr(s, w)) return 1;

   } while(p);

   return 0;
}


/*
 * is it a valid dotted IPv4 address
 */

int is_dotted_ipv4_address(char *s){
   struct in_addr addr;

   if(inet_aton(s, &addr) == 0) return 0;

   return 1;
}


#ifndef _GNU_SOURCE
char *strcasestr(const char *s, const char *find){
   char c, sc;
   size_t len;

   if((c = *find++) != 0){
      c = tolower((unsigned char)c);
      len = strlen(find);
      do {
         do {
            if((sc = *s++) == 0)
               return (NULL);
         } while((char)tolower((unsigned char)sc) != c);
      } while (strncasecmp(s, find, len) != 0);
      s--;
   }

   return((char*)s);
}
#endif
