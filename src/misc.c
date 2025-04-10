/*
 * misc.c, SJ
 */

#include <kospam.h>


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


void init_session_data(struct session_data *sdata){
   memset((char*)sdata, 0, sizeof(*sdata));

   sdata->rav = AVIR_OK;

   sdata->spaminess = DEFAULT_SPAMICITY;
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

   while(p) {
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

   }

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

long get_local_timezone_offset(){
   time_t t = time(NULL);
   struct tm lt = {0};
   localtime_r(&t, &lt);
   return lt.tm_gmtoff;
}
