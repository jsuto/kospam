/*
 * clapf_admin.c, 2007.09.21, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "misc.h"
#include "hash.h"
#include "decoder.h"
#include "parser.h"
#include "errmsg.h"
#include "messages.h"
#include "config.h"

#ifdef HAVE_MYSQL
   #include <mysql.h>
   MYSQL mysql;
   MYSQL_RES *res;
   MYSQL_ROW row;
#endif

#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;
   int rc;
#endif

extern char *optarg;
extern int optind;


/*
 * determines next uid
 */

unsigned long next_uid(){
   unsigned long uid=0;
   char stmt[MAXBUFSIZE];

   snprintf(stmt, MAXBUFSIZE-1, "SELECT MAX(uid)+1 FROM %s", SQL_USER_TABLE);

#ifdef HAVE_MYSQL
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
#endif
#ifdef HAVE_SQLITE3
   if(sqlite3_prepare_v2(db, stmt, -1, &pStmt, ppzTail) == SQLITE_OK){
      if(sqlite3_step(pStmt) == SQLITE_ROW)
         uid = sqlite3_column_int(pStmt, 0);
   }

   sqlite3_finalize(pStmt);
#endif

   if(uid == 0) uid = 1;

   return uid;
}


int main(int argc, char **argv){
   unsigned long uid=-1;
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
                    if(uid < 0){
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


#ifdef HAVE_MYSQL
   mysql_init(&mysql);
   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0))
      __fatal(ERR_MYSQL_CONNECT);
#endif
#ifdef HAVE_SQLITE3
   rc = sqlite3_open(cfg.sqlite3, &db);
   if(rc)
      __fatal(ERR_SQLITE3_OPEN);
#endif


   /* add user */

   if(uid == -1) uid = next_uid();

   snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (username, email, uid, action) VALUES('%s', '%s', %ld, '%s')", SQL_USER_TABLE, username, email, uid, action);

#ifdef HAVE_MYSQL
   mysql_real_query(&mysql, buf, strlen(buf));

   if(mysql_affected_rows(&mysql) != 1){
      printf("%s: %ld\n", ERR_EXISTING_UID, uid);
      goto END;
   }
#endif
#ifdef HAVE_SQLITE3
   sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail);
   rc = sqlite3_step(pStmt);
   sqlite3_finalize(pStmt);

   if(rc != SQLITE_DONE){
      printf("%s: %ld\n", ERR_EXISTING_UID, uid);
      goto END;
   }
#endif

   if(uid > 0){
      snprintf(buf, MAXBUFSIZE-1, "INSERT INTO %s (nham, nspam, uid) VALUES(0, 0, %ld)", SQL_MISC_TABLE, uid);

   #ifdef HAVE_MYSQL
      mysql_real_query(&mysql, buf, strlen(buf));
   #endif
   #ifdef HAVE_SQLITE3
      sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail);
      sqlite3_step(pStmt);
      sqlite3_finalize(pStmt);
   #endif
   }

END:

#ifdef HAVE_MYSQL
   mysql_close(&mysql);
#endif
#ifdef HAVE_SQLITE3
   sqlite3_close(db);
#endif

   return 0;
}
