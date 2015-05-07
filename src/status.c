/*
 * users.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <clapf.h>


void init_child_stat_entry(struct session_data *sdata){
   char buf[SMALLBUFSIZE];

   snprintf(buf, sizeof(buf)-1, "DELETE FROM %s", SQL_STATUS_TABLE);
   //syslog(LOG_PRIORITY, "status=%s", buf);
   p_query(sdata, buf);
}


void create_child_stat_entry(struct session_data *sdata, pid_t pid){
   char buf[SMALLBUFSIZE];

   time(&(sdata->now));

   snprintf(buf, sizeof(buf)-1, "INSERT INTO %s (pid, status, ts) VALUES(%d, 'I', %ld)", SQL_STATUS_TABLE, pid, sdata->now);
   //syslog(LOG_PRIORITY, "status=%s", buf);
   p_query(sdata, buf);
}


void remove_child_stat_entry(struct session_data *sdata, pid_t pid){
   char buf[SMALLBUFSIZE];

   snprintf(buf, sizeof(buf)-1, "DELETE FROM %s WHERE pid=%d", SQL_STATUS_TABLE, pid);
   //syslog(LOG_PRIORITY, "status=%s", buf);
   p_query(sdata, buf);
}


void update_child_stat_entry(struct session_data *sdata, char status, int count){
   char buf[SMALLBUFSIZE];

   time(&(sdata->now));

   if(count > 0) snprintf(buf, sizeof(buf)-1, "UPDATE %s SET status='%c', ts=%ld, messages=%d WHERE pid=%d", SQL_STATUS_TABLE, status, sdata->now, count, sdata->pid);
   else snprintf(buf, sizeof(buf)-1, "UPDATE %s SET status='%c', ts=%ld WHERE pid=%d", SQL_STATUS_TABLE, status, sdata->now, sdata->pid);

   //syslog(LOG_PRIORITY, "status=%s", buf);
   p_query(sdata, buf);
}

