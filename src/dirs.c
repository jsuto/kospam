/*
 * dirs.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <clapf.h>


void createdir(char *path, mode_t mode){
   struct stat st;

   if(strlen(path) > 2){
      if(path[strlen(path)-1] == '/') path[strlen(path)-1] = '\0';

      if(stat(path, &st)){
         if(mkdir(path, mode) == 0){
            syslog(LOG_PRIORITY, "created directory: *%s*", path);
         }
         else syslog(LOG_PRIORITY, "error: could not create directory '%s'", path);
      }

   }
}


void check_and_create_directories(struct __config *cfg){
   char *p;

   p = strrchr(cfg->workdir, '/');
   if(p){
      *p = '\0';
      createdir(cfg->workdir, 0755);
      *p = '/';
   }
   createdir(cfg->workdir, 0711);

   p = strrchr(cfg->queuedir, '/');
   if(p){
      *p = '\0';
      createdir(cfg->queuedir, 0755);
      *p = '/';
   }
   createdir(cfg->queuedir, 0700);

   p = strrchr(cfg->pidfile, '/');
   if(p){
      *p = '\0';
      createdir(cfg->pidfile, 0755);
      *p = '/';
   }

   for(int i=0; i<cfg->number_of_worker_processes; i++){
      char s[SMALLBUFSIZE];
      snprintf(s, sizeof(s)-1, "%s/%d", cfg->workdir, i);
      createdir(s, 0700);
   }

   createdir(HISTORY_DIR, 0700);
   createdir(HISTORY_DIR "/tmp", 0700);
   createdir(HISTORY_DIR "/new", 0700);
}
