/*
 * clapf_admin.c, 2007.06.27, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <unistd.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "config.h"

extern char *optarg;
extern int optind;

MYSQL mysql;


/*
 * determines next uid
 */

unsigned long next_uid(char *mysqlusertable){
   MYSQL_RES *res;
   MYSQL_ROW row;
   unsigned long uid=0;
   char stmt[MAXBUFSIZE];

   snprintf(stmt, MAXBUFSIZE-1, "SELECT MAX(uid)+1 FROM %s", mysqlusertable);

   if(mysql_real_query(&mysql, stmt, strlen(stmt)) == 0){
      res = mysql_store_result(&mysql);
      if(res){
         row = mysql_fetch_row(res);
         if(row){
            if(row[0]) uid = atol(row[0]);
         }
         mysql_free_result(res);
      }
   }

   if(uid == 0) uid = 1;

   return uid;
}


int main(int argc, char **argv){
   unsigned long uid=0;
   char buf[MAXBUFSIZE], *username=NULL, *email=NULL, *action="junk";
   char *configfile=CONFIG_FILE;
   int i;
   struct __config cfg;

   while((i = getopt(argc, argv, "c:u:e:a:i:h")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'u' :
                    username = optarg;
                    break;

         case 'e' :
                    email = optarg;
                    break;

         case 'a' :
                    action = optarg;
                    break;

         case 'i' :
                    uid = atol(optarg);
                    if(uid < 1){
                       printf("%s: %ld\n", ERR_INVALID_UID, uid);
                       return 1;
                    }

                    break;

         case 'h' :
         default  : 
                    __fatal(ADMINUSAGE);
       }
   }

   cfg = read_config(configfile);

   if(username == NULL || email == NULL) __fatal(ADMINUSAGE);


   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      __fatal(ERR_MYSQL_CONNECT);


   /* add user */

   if(uid == 0) uid = next_uid(SQL_USER_TABLE);

   snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (username, email, uid, action) VALUES('%s', '%s', %ld, '%s')", SQL_USER_TABLE, username, email, uid, action);
   mysql_real_query(&mysql, buf, strlen(buf));

   if(mysql_affected_rows(&mysql) != 1){
      printf("%s: %ld\n", ERR_EXISTING_UID, uid);
      goto END;
   }

   snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (nham, nspam, uid) VALUES(0, 0, %ld)", SQL_MISC_TABLE, uid);
   mysql_real_query(&mysql, buf, strlen(buf));

END:
   mysql_close(&mysql);


   return 0;
}
