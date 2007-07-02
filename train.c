/*
 * train.c, 2007.06.28, SJ
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

void my_walk_hash(MYSQL mysql, int ham_or_spam, char *tokentable, struct node *xhash[MAXHASH], unsigned int uid, int train_mode);

int main(int argc, char **argv){
   char buf[MAXBUFSIZE];
   char *configfile=CONFIG_FILE, *messagefile=NULL;
   int i, m, addspam=0, addham=0, is_spam=0;
   unsigned long cnt;
   struct _state state;
   struct _token *P, *Q;
   struct __config cfg;
   struct node *tokens[MAXHASH];
   FILE *F;

   while((i = getopt(argc, argv, "c:S:H:h")) > 0){
       switch(i){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'S' :
                    addspam = 1;
                    is_spam = 1;
                    messagefile = optarg;
                    break;

         case 'H' :
                    addham = 1;
                    messagefile = optarg;
                    break;


         case 'h' :
         default  : 
                    __fatal(TRAINUSAGE);
       }
   }

   if(addham == 0 && addspam == 0)
      __fatal(ERR_INVALID_MODE ": " TRAINUSAGE);

   if(addham == 1 && addspam == 1)
      __fatal(ERR_INVALID_MODE ": " TRAINUSAGE);


   if(messagefile == NULL)
       __fatal(TRAINUSAGE);

   cfg = read_config(configfile);


   state = init_state();

   /* read message file */

   F = fopen(messagefile, "r");
   if(!F)
      __fatal(ERR_OPEN_MESSGAE_FILE);

   while(fgets(buf, MAXBUFSIZE-1, F))
      state = parse(buf, state);


   mysql_init(&mysql);

   if(!mysql_real_connect(&mysql, cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0)){
      if(state.first)
         free_and_print_list(state.first, 0);

      __fatal(ERR_MYSQL_CONNECT);
   }

   inithash(tokens);

   if(state.first){

      P = state.first;
      cnt = 0;

      while(P != NULL){
         Q = P->r;
         m = 1;

         addnode(tokens, P->str, 0, 0);

         if(P)
            free(P);

         P = Q;

         cnt++;
      }


      my_walk_hash(mysql, is_spam, cfg.mysqltokentable, tokens, 0, T_TOE);


      /* update the t_misc table */

      if(is_spam == 1)
         snprintf(buf, MAXBUFSIZE-1, "update %s set update_cdb=1, nspam=nspam+1", cfg.mysqlmisctable);
      else
         snprintf(buf, MAXBUFSIZE-1, "update %s set update_cdb=1, nham=nham+1", cfg.mysqlmisctable);

      mysql_real_query(&mysql, buf, strlen(buf));


      mysql_close(&mysql);
   }

   clearhash(tokens);


   printf("%s\n", ERR_TRAIN_DONE);

   return 0;
}
