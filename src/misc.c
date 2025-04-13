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


/*
 * is it a valid dotted IPv4 address
 */

int is_dotted_ipv4_address(char *s){
   struct in_addr addr;

   if(inet_aton(s, &addr) == 0) return 0;

   return 1;
}
