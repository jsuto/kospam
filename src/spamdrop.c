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
   char *p;
   char *configfile=CONFIG_FILE;
   char *message=NULL;
   char *from=NULL, *recipient=NULL;
   struct stat st;
   struct session_data sdata;
   struct __state state;
   struct __config cfg, my_cfg;
   struct __data data;
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
                    recipient = optarg;
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

   if(!can_i_write_current_directory()) __fatal("cannot write current directory!");

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

   if(open_database(&sdata, &cfg) == ERR) return 0;

   if(cfg.debug == 1) printf("locale: %s\n", setlocale(LC_MESSAGES, cfg.locale));
   setlocale(LC_CTYPE, cfg.locale);

   data.n_regex = 0;

   zombie_init(&data, &cfg);

   init_session_data(&sdata, &cfg);

   sdata.sent = 0;
   sdata.tot_len = st.st_size;
   sdata.num_of_rcpt_to = 1;

   snprintf(sdata.ttmpfile, SMALLBUFSIZE-1, "%s", message);
   snprintf(sdata.filename, SMALLBUFSIZE-1, "%s", message);

   memcpy(&my_cfg, &cfg, sizeof(struct __config));

   if(recipient){
      if(cfg.debug == 1) printf("checking user data...\n");

      if(get_user_data_from_email(&sdata, recipient, &cfg) == 0){
         p = strchr(recipient, '@');
         if(p) get_user_data_from_email(&sdata, p, &cfg);
      }

      if(sdata.uid > 0 && cfg.debug == 1) printf("uid=%d, gid=%d, username='%s', domain='%s', policygroup=%d\n", sdata.uid, sdata.gid, sdata.name, sdata.domain, sdata.policy_group);
   }

   if(cfg.debug == 1) printf("policy id: %d\n", sdata.policy_group);

   if(sdata.policy_group > 0){
      get_policy(&sdata, &cfg, &my_cfg);
      if(cfg.debug == 1) printf("policy settings: %d/%d/%d/%s/%d/%s/%.4f/%.4f/%d/%d/%d/%d/%d/%d/%d/%d/%s\n", my_cfg.deliver_infected_email, my_cfg.silently_discard_infected_email, my_cfg.use_antispam, my_cfg.spam_subject_prefix, my_cfg.max_message_size_to_filter, my_cfg.surbl_domain, my_cfg.spam_overall_limit, my_cfg.spaminess_oblivion_limit, my_cfg.replace_junk_characters, my_cfg.penalize_images, my_cfg.penalize_embed_images, my_cfg.penalize_octet_stream, my_cfg.training_mode, my_cfg.store_emails, my_cfg.store_only_spam, my_cfg.message_from_a_zombie, my_cfg.smtp_addr);
   }


   if(is_item_on_list(from, sdata.whitelist, "") == 1){
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

      train_message(&sdata, &state, s, &my_cfg);
   }

   check_zombie_sender(&state, &data, &my_cfg);

   struct timeval tv_spam1, tv_spam2;
   gettimeofday(&tv_spam1, &tz);
   sdata.spaminess = run_statistical_check(&sdata, &state, &my_cfg);
   gettimeofday(&tv_spam2, &tz);

CLEANUP:
   clearhash(state.token_hash);
   clearhash(state.url);

   close_database(&sdata);

   zombie_free(&data);

   gettimeofday(&tv_stop, &tz);

   if(cfg.debug == 1){
      printf("spaminess: %.4f in %ld/%ld [ms]\n", sdata.spaminess, tvdiff(tv_spam2, tv_spam1)/1000, tvdiff(tv_stop, tv_start)/1000);
      printf("rcvd host/ip/zombie: %s/%s/%c\n", state.hostname, state.ip, state.tre);
      printf("number of tokens: %d/%d\n", state.n_token, state.n_deviating_token);
   }

   return 0;
}
