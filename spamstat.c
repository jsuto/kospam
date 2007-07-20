/*
 * spamstat.c, 2007.05.18, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <mysql.h>
#include "misc.h"
#include "cfg.h"
#include "messages.h"
#include "config.h"


int main(int argc, char **argv){
   char *p, buf[MAXBUFSIZE], puf[MAXBUFSIZE];
   unsigned long uid, now, nham, nspam;
   struct __config cfg;
   FILE *f;
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;

   if(argc < 2)
      cfg = read_config(CONFIG_FILE);
   else
      cfg = read_config(argv[1]);

   time(&now);

   mysql_init(&mysql);
   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      __fatal(ERR_MYSQL_CONNECT);
 
   f = fopen("/dev/stdin", "r");
   if(!f)
      __fatal(ERR_CANNOT_OPEN);


   while(fgets(buf, MAXBUFSIZE-1, f)){
      //email@address 5 2

      nham = nspam = 0;

      p = strrchr(buf, ' ');
      if(p){
         *p = '\0';
         nspam = atol(p+1);
         p = strrchr(buf, ' ');

         if(p){
            *p = '\0';
             nham = atol(p+1);
         }
      }

      if(nham > 0 || nspam > 0){

         /* get uid */

         uid = 0;

         snprintf(puf, MAXBUFSIZE-1, "SELECT uid FROM %s WHERE email='%s'", SQL_USER_TABLE, buf);

         if(mysql_real_query(&mysql, puf, strlen(puf)) == 0){
            res = mysql_store_result(&mysql);
            if(res != NULL){
               row = mysql_fetch_row(res);
               if(row)
                  uid = atol(row[0]);
               mysql_free_result(res);
            }
         }

         if(uid > 0){
            snprintf(puf, MAXBUFSIZE-1, "INSERT INTO %s (uid, ts, nham, nspam) VALUES(%ld, %ld, %ld, %ld)", SQL_STAT_TABLE, uid, now, nham, nspam);
            mysql_real_query(&mysql, puf, strlen(puf));
         }
      }
   }

   fclose(f);
   mysql_close(&mysql);

   return 0;
}

