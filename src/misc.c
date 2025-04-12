/*
 * misc.c, SJ
 */

#include <kospam.h>


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
   sdata->spaminess = DEFAULT_SPAMICITY;
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

      chop_newlines(w, strlen(w));

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


/*
 * is it a valid dotted IPv4 address
 */

int is_dotted_ipv4_address(char *s){
   struct in_addr addr;

   if(inet_aton(s, &addr) == 0) return 0;

   return 1;
}
