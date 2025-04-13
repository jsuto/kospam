/*
 * spamdrop.c, SJ
 */

#include <kospam.h>


extern char *optarg;
extern int optind;


void usage(){
   printf("\nusage: spamdrop\n\n");
   printf("    [-c|--config <config file>]\n");
   printf("    [-m|--message <EML file>]\n");
   printf("    [-f|--from <from address>]\n");
   printf("    [-r|--recipient <recipient address>]\n");
   printf("    [-H|--ham]\n");
   printf("    [-S|--spam]\n");
   printf("    [-D|--debug]\n");
   printf("    [-t|--tokens]\n");
   printf("    [-v|--version]\n");
   printf("\n");
   exit(1);
}


int main(int argc, char **argv){
   int c, debug=0;
   int train_as_ham=0, train_as_spam=0;
   int show_tokens=0;
   char *configfile=CONFIG_FILE;
   char *message=NULL;
   char *from=NULL;
   //char *recipient=NULL;
   struct stat st;
   struct session_data sdata;
   struct parser_state state;
   struct config cfg;
   struct data data;
   struct timezone tz;
   struct timeval tv_start, tv_stop;

   while(1){

#ifdef _GNU_SOURCE
      static struct option long_options[] =
         {
            {"config",       required_argument,  0,  'c' },
            {"from",         required_argument,  0,  'f' },
            {"recipient",    required_argument,  0,  'r' },
            {"message",      required_argument,  0,  'm' },
            {"ham",          no_argument,        0,  'H' },
            {"spam",         no_argument,        0,  'S' },
            {"tokens",       no_argument,        0,  't' },
            {"debug",        no_argument,        0,  'D' },
            {"help",         no_argument,        0,  'h' },
            {"version",      no_argument,        0,  'v' },
            {0,0,0,0}
         };

      int option_index = 0;

      c = getopt_long(argc, argv, "c:u:U:f:r:m:HStDv?", long_options, &option_index);
#else
      c = getopt(argc, argv, "c:u:U:f:r:m:HStDv?");
#endif

      if(c == -1) break;

       switch(c){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'f' :
                    from = optarg;
                    break;

         case 'r' :
                    //recipient = optarg;
                    break;

         case 'S' :
                    train_as_spam = 1;
                    break;

         case 'H' :
                    train_as_ham = 1;
                    break;

         case 't' :
                    show_tokens = 1;
                    break;

         case 'D' :
                    debug = 1;
                    break;

         case 'm' :
                    message = optarg;
                    break;

         case 'h' :
         case '?' :
                    usage();
                    break;


         default  :
                    break;
       }
   }


   if(message == NULL) usage();

   if(train_as_spam == 1 && train_as_ham == 1){
      fprintf(stderr, "cannot train a message as ham and spam at the same time\n");
      return 1;
   }

   srand(getpid());

   if(stat(message, &st) != 0){
      fprintf(stderr, "%s is not found\n", message);
      return 1;
   }

   (void) openlog("spamdrop", LOG_PID, LOG_MAIL);

   cfg = read_config(configfile);

   if(debug == 1){
      cfg.verbosity = 0;
      cfg.debug = 1;
   }

   gettimeofday(&tv_start, &tz);

   MYSQL *conn = open_database(&cfg);
   if (conn == NULL) return 0;

   if(cfg.debug == 1) printf("locale: %s\n", setlocale(LC_MESSAGES, cfg.locale));
   setlocale(LC_CTYPE, cfg.locale);

   data.n_regex = 0;

   zombie_init(&data, &cfg);

   init_session_data(&sdata);

   sdata.tot_len = st.st_size;

   snprintf(sdata.ttmpfile, SMALLBUFSIZE-1, "%s", message);

   if(is_item_on_list(from, sdata.whitelist)){
      if(cfg.debug == 1) printf(" '%s' found on '%s'\n", from, sdata.whitelist);
   }

   if(cfg.debug == 1) printf("parsing...\n");

   struct Message m;

   parse_message(message, &state, &m);
   post_parse(&state, &m, &cfg);

   if(show_tokens == 1){
      printhash(state.token_hash);
      goto CLEANUP;
   }

   if(train_as_ham == 1 || train_as_spam == 1){
      char s[SMALLBUFSIZE];
      if(train_as_spam == 1) snprintf(s, sizeof(s)-1, "nspam");
      else snprintf(s, sizeof(s)-1, "nham");

      train_message(&state, s, &cfg);
   }

   check_zombie_sender(&state, &data, &cfg);

   struct timeval tv_spam1, tv_spam2;
   gettimeofday(&tv_spam1, &tz);
   sdata.spaminess = run_statistical_check(&sdata, &state, conn, &cfg);
   gettimeofday(&tv_spam2, &tz);

CLEANUP:
   clearhash(state.token_hash);
   clearhash(state.url);

   close_database(conn);

   zombie_free(&data);

   gettimeofday(&tv_stop, &tz);

   if(cfg.debug == 1){
      printf("spaminess: %.4f in %ld/%ld [ms]\n", sdata.spaminess, tvdiff(tv_spam2, tv_spam1)/1000, tvdiff(tv_stop, tv_start)/1000);
      printf("rcvd host/ip/zombie: %s/%s/%c\n", state.hostname, state.ip, state.tre);
      printf("number of tokens: %d/%d\n", state.n_token, state.n_deviating_token);
   }

   return 0;
}
