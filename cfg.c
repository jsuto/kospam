/*
 * cfg.c, 2007.06.13, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfg.h"
#include "config.h"


/*
 * extract a string 's' from string 'row'
 */

char *extract(char *row, int ch, char *s, int size){
   char *r;
   int len;

   if(row == NULL) return NULL;

   if((r = strchr(row, ch)) == NULL) return NULL;
   if(s != NULL){
       len = strlen(row) - strlen(r);
       if(len > size) len = size;
       strncpy(s, row, len);
       s[len] = '\0';
   }

   r++;
   return r;
}


/*
 * read configuration file variables
 */

struct __config read_config(char *configfile){
   struct __config cfg;
   char *p, buf[MAXBUFSIZE], key[MAXVAL], val[MAXVAL];
   FILE *F;

   /* reset config structure and fill it with defaults */

   memset((char *)&cfg, 0, sizeof(struct __config));

   strncpy(cfg.hostid, HOSTID, MAXVAL-1);

   strncpy(cfg.listen_addr, LISTEN_ADDR, MAXVAL-1);
   cfg.listen_port = LISTEN_PORT;

   strncpy(cfg.postfix_addr, POSTFIX_ADDR, MAXVAL-1);
   cfg.postfix_port = POSTFIX_PORT;

   strncpy(cfg.spam_smtp_addr, POSTFIX_ADDR, MAXVAL-1);
   cfg.spam_smtp_port = POSTFIX_PORT;

   strncpy(cfg.avg_addr, AVG_ADDR, MAXVAL-1);
   cfg.avg_port = AVG_PORT;

   strncpy(cfg.avast_addr, AVAST_ADDR, MAXVAL-1);
   cfg.avast_port = AVAST_PORT;

   strncpy(cfg.kav_socket, KAV_SOCKET, MAXVAL-1);

   strncpy(cfg.drweb_socket, DRWEB_SOCKET, MAXVAL-1);

   strncpy(cfg.clamd_socket, CLAMD_SOCKET, MAXVAL-1);

   strncpy(cfg.qcache_addr, QCACHE_ADDR, MAXVAL-1);
   cfg.qcache_port = QCACHE_PORT;
   strncpy(cfg.qcache_socket, QCACHE_SOCKET, MAXVAL-1);

   cfg.max_connections = MAXCONN;
   cfg.backlog = BACKLOG;

   cfg.session_timeout = SESSION_TIMEOUT;

   cfg.silently_discard_infected_email = 1;

   memset(cfg.chrootdir, 0, MAXVAL);

   strncpy(cfg.workdir, WORK_DIR, MAXVAL-1);

   strncpy(cfg.surbl_domain, SURBL_DOMAIN, MAXVAL-1);
   cfg.rude_surbl = 0;

   strncpy(cfg.clapf_header_field, SPAMINESS_HEADER_FIELD, MAXVAL-1);

   cfg.spam_overall_limit = 0.92;
   cfg.spaminess_oblivion_limit=1.01;
   cfg.min_deviation_to_use_single_tokens = 0.1;
   cfg.use_single_tokens_min_limit = 0.85;

   cfg.spaminess_of_strange_language_stuff = 0.9876;
   cfg.spaminess_of_too_much_spam_in_top15 = 0.9998;
   cfg.spaminess_of_blackholed_mail = 0.9995;
   cfg.spaminess_of_text_and_base64 = 0.9996;
   cfg.spaminess_of_caught_by_surbl = 0.9997;
   cfg.spaminess_of_embed_image = 0.9994;

   cfg.use_all_the_most_interesting_tokens = 1;
   cfg.spam_ratio_in_top10 = 0.9;

   cfg.invalid_junk_limit = INVALID_JUNK_LIMIT;
   cfg.invalid_junk_line = INVALID_JUNK_LINE;
   cfg.invalid_hex_junk_limit = INVALID_HEX_JUNK_LIMIT;
   cfg.max_junk_spamicity = MAX_JUNK_SPAMICITY;

   //cfg.mysqlport = 3306;

   cfg.verbosity = 0;

   cfg.use_antispam = 1;


   strncpy(cfg.tokensfile, TOKENSCDB, MAXVAL-1);

   cfg.use_pairs = 1;
   cfg.use_single_tokens = 1;

   cfg.min_phrase_number = MIN_PHRASE_NUMBER;

   cfg.enable_auto_white_list = 1;

   cfg.esf_h = 1;
   cfg.esf_s = 1;

   cfg.exclusion_radius = EXCLUSION_RADIUS;

   cfg.max_message_size_to_filter = 0;

   cfg.num_of_skip_headers = 0;

   cfg.relocate_delay = 5;

   cfg.use_libclamav_block_max_feature = 1;
   cfg.clamav_maxfile = MAXFILES;
   cfg.clamav_max_archived_file_size = MAX_ARCHIVED_FILE_SIZE;
   cfg.clamav_max_recursion_level = MAX_RECURSION_LEVEL;
   cfg.clamav_max_compress_ratio = MAX_COMPRESS_RATIO;
   cfg.clamav_archive_mem_limit = ARCHIVE_MEM_LIMIT;

   cfg.page_len = MESSAGES_PER_ONE_PAGE;

   /* parse the config file */

   if(configfile){
      F = fopen(configfile, "r");
      if(F){
         while(fgets(buf, MAXBUFSIZE, F)){
            if(buf[0] != ';' && buf[0] != '#'){
               memset(key, 0, MAXVAL);
               memset(val, 0, MAXVAL);

               p = extract(buf, '=', key, MAXVAL-1);
               p = extract(p, '\n', val, MAXVAL-1);


               if(p){
                  if(strcmp(key, "hostid") == 0)
                     memcpy(cfg.hostid, val, MAXVAL-1);

                  if(strcmp(key, "listen_addr") == 0)
                     memcpy(cfg.listen_addr, val, MAXVAL-1);

                  if(strcmp(key, "listen_port") == 0)
                     cfg.listen_port = atoi(val);

                  if(strcmp(key, "postfix_addr") == 0)
                     memcpy(cfg.postfix_addr, val, MAXVAL-1);

                  if(strcmp(key, "postfix_port") == 0)
                     cfg.postfix_port = atoi(val);

                  if(strcmp(key, "avg_addr") == 0)
                     memcpy(cfg.avg_addr, val, MAXVAL-1);

                  if(strcmp(key, "avg_port") == 0)
                     cfg.avg_port = atoi(val);

                  if(strcmp(key, "avast_addr") == 0)
                     memcpy(cfg.avast_addr, val, MAXVAL-1);

                  if(strcmp(key, "avast_port") == 0)
                     cfg.avast_port = atoi(val);

                  if(strcmp(key, "kav_socket") == 0)
                     memcpy(cfg.kav_socket, val, MAXVAL-1);

                  if(strcmp(key, "drweb_socket") == 0)
                     memcpy(cfg.drweb_socket, val, MAXVAL-1);

                  if(strcmp(key, "clamd_socket") == 0)
                     memcpy(cfg.clamd_socket, val, MAXVAL-1);

                  if(strcmp(key, "qcache_addr") == 0)
                     memcpy(cfg.qcache_addr, val, MAXVAL-1);

                  if(strcmp(key, "qcache_port") == 0)
                     cfg.qcache_port = atoi(val);

                  if(strcmp(key, "qcache_socket") == 0)
                     memcpy(cfg.qcache_socket, val, MAXVAL-1);

                  if(strcmp(key, "max_connections") == 0)
                     cfg.max_connections = atoi(val);

                  if(strcmp(key, "backlog") == 0)
                     cfg.backlog = atoi(val);

                  if(strcmp(key, "chrootdir") == 0)
                     memcpy(cfg.chrootdir, val, MAXVAL-1);

                  if(strcmp(key, "workdir") == 0)
                     memcpy(cfg.workdir, val, MAXVAL-1);

                  if(strcmp(key, "queuedir") == 0)
                     memcpy(cfg.queuedir, val, MAXVAL-1);

                  if(strcmp(key, "quarantine_dir") == 0)
                     memcpy(cfg.quarantine_dir, val, MAXVAL-1);

                  if(strcmp(key, "blackhole_path") == 0)
                     memcpy(cfg.blackhole_path, val, MAXVAL-1);

                  if(strcmp(key, "verbosity") == 0)
                     cfg.verbosity = atoi(val);

                  if(strcmp(key, "session_timeout") == 0)
                     cfg.session_timeout = atoi(val);

                  if(strcmp(key, "clapfemail") == 0)
                     memcpy(cfg.clapfemail, val, MAXVAL-1);

                  if(strcmp(key, "localpostmaster") == 0)
                     memcpy(cfg.localpostmaster, val, MAXVAL-1);

                  if(strcmp(key, "silently_discard_infected_email") == 0)
                     cfg.silently_discard_infected_email = atoi(val);

                  if(strcmp(key, "use_antispam") == 0)
                     cfg.use_antispam = atoi(val);


                  if(strcmp(key, "userprefdb") == 0)
                     memcpy(cfg.userprefdb, val, MAXVAL-1);

                  if(strcmp(key, "spam_subject_prefix") == 0)
                     memcpy(cfg.spam_subject_prefix, val, MAXVAL-1);


                  if(strcmp(key, "spam_quarantine_dir") == 0)
                     memcpy(cfg.spam_quarantine_dir, val, MAXVAL-1);

                  if(strcmp(key, "tokensfile") == 0)
                     memcpy(cfg.tokensfile, val, MAXVAL-1);

                  if(strcmp(key, "use_triplets") == 0)
                     cfg.use_triplets = atoi(val);

                  if(strcmp(key, "use_pairs") == 0)
                     cfg.use_pairs = atoi(val);

                  if(strcmp(key, "use_single_tokens") == 0)
                     cfg.use_single_tokens = atoi(val);

                  if(strcmp(key, "min_phrase_number") == 0)
                     cfg.min_phrase_number = atoi(val);

                  if(strcmp(key, "enable_auto_white_list") == 0)
                     cfg.enable_auto_white_list = atoi(val);

                  if(strcmp(key, "esf_h") == 0)
                     cfg.esf_h = atof(val);

                  if(strcmp(key, "esf_s") == 0)
                     cfg.esf_s = atof(val);

                  if(strcmp(key, "exclusion_radius") == 0)
                     cfg.exclusion_radius = atof(val);

                  if(strcmp(key, "max_message_size_to_filter") == 0)
                     cfg.max_message_size_to_filter = atol(val);

                  if(strcmp(key, "surbl_domain") == 0)
                     memcpy(cfg.surbl_domain, val, MAXVAL-1);

                  if(strcmp(key, "rude_surbl") == 0)
                     cfg.rude_surbl = atoi(val);


                  if(strcmp(key, "clapf_header_field") == 0)
                     memcpy(cfg.clapf_header_field, val, MAXVAL-1);

                  /* allow adding multiple header lines in case of spam, credits: Mariano, 2006.08.14 */

                  if(strcmp(key, "clapf_spam_header_field") == 0){
                     strncat(cfg.clapf_spam_header_field, val, MAXVAL-1);
                     strncat(cfg.clapf_spam_header_field, "\r\n", MAXVAL-1);
                  }

                  if(strcmp(key, "spam_overall_limit") == 0)
                     cfg.spam_overall_limit = atof(val);

                  if(strcmp(key, "spaminess_oblivion_limit") == 0)
                     cfg.spaminess_oblivion_limit = atof(val);

                  if(strcmp(key, "use_single_tokens_min_limit") == 0)
                     cfg.use_single_tokens_min_limit = atof(val);

                  if(strcmp(key, "min_deviation_to_use_single_tokens") == 0)
                     cfg.min_deviation_to_use_single_tokens = atof(val);

                  if(strcmp(key, "skip_headers") == 0)
                     memcpy(cfg.skip_headers, val, MAXVAL-1);

                  if(strcmp(key, "spaminess_of_strange_language_stuff") == 0)
                     cfg.spaminess_of_strange_language_stuff = atof(val);

                  if(strcmp(key, "spaminess_of_too_much_spam_in_top15") == 0)
                     cfg.spaminess_of_too_much_spam_in_top15 = atof(val);

                  if(strcmp(key, "spaminess_of_blackholed_mail") == 0)
                     cfg.spaminess_of_blackholed_mail = atof(val);

                  if(strcmp(key, "spaminess_of_text_and_base64") == 0)
                     cfg.spaminess_of_text_and_base64 = atof(val);

                  if(strcmp(key, "spaminess_of_caught_by_surbl") == 0)
                     cfg.spaminess_of_caught_by_surbl = atof(val);

                  if(strcmp(key, "spaminess_of_embed_image") == 0)
                     cfg.spaminess_of_embed_image = atof(val);

                  if(strcmp(key, "use_all_the_most_interesting_tokens") == 0)
                     cfg.use_all_the_most_interesting_tokens = atoi(val);

                  if(strcmp(key, "spam_ratio_in_top10") == 0)
                     cfg.spam_ratio_in_top10 = atof(val);

                  if(strcmp(key, "invalid_junk_limit") == 0)
                     cfg.invalid_junk_limit = atoi(val);

                  if(strcmp(key, "invalid_junk_line") == 0)
                     cfg.invalid_junk_line = atoi(val);

                  if(strcmp(key, "invalid_hex_junk_limit") == 0)
                     cfg.invalid_hex_junk_limit = atoi(val);

                  if(strcmp(key, "max_junk_spamicity") == 0)
                     cfg.max_junk_spamicity = atof(val);

                  if(strcmp(key, "penalize_images") == 0)
                     cfg.penalize_images = atoi(val);

                  if(strcmp(key, "penalize_embed_images") == 0)
                     cfg.penalize_embed_images = atoi(val);

                  if(strcmp(key, "penalize_octet_stream") == 0)
                     cfg.penalize_octet_stream = atoi(val);

                  if(strcmp(key, "max_embed_image_spamicity") == 0)
                     cfg.max_embed_image_spamicity = atof(val);

                  if(strcmp(key, "training_mode") == 0)
                     cfg.training_mode = atoi(val);

                  if(strcmp(key, "group_type") == 0)
                     cfg.group_type = atoi(val);


                  if(strcmp(key, "use_libclamav_block_max_feature") == 0)
                     cfg.use_libclamav_block_max_feature = atoi(val);

                  if(strcmp(key, "clamav_maxfile") == 0)
                     cfg.clamav_maxfile = atoi(val);

                  if(strcmp(key, "clamav_max_archived_file_size") == 0)
                     cfg.clamav_max_archived_file_size = atol(val);
  
                  if(strcmp(key, "clamav_max_recursion_level") == 0) 
                     cfg.clamav_max_recursion_level = atoi(val);

                  if(strcmp(key, "clamav_max_compress_ratio") == 0)
                     cfg.clamav_max_compress_ratio = atoi(val);

                  if(strcmp(key, "clamav_archive_mem_limit") == 0)                 
                     cfg.clamav_archive_mem_limit = atoi(val);



                  if(strcmp(key, "mysqlhost") == 0)
                     memcpy(cfg.mysqlhost, val, MAXVAL-1);

                  if(strcmp(key, "mysqlport") == 0)
                     cfg.mysqlport = atoi(val);

                  if(strcmp(key, "mysqlsocket") == 0)
                     memcpy(cfg.mysqlsocket, val, MAXVAL-1);

                  if(strcmp(key, "mysqluser") == 0)
                     memcpy(cfg.mysqluser, val, MAXVAL-1);

                  if(strcmp(key, "mysqlpwd") == 0)
                     memcpy(cfg.mysqlpwd, val, MAXVAL-1);

                  if(strcmp(key, "mysqldb") == 0)
                     memcpy(cfg.mysqldb, val, MAXVAL-1);

                  if(strcmp(key, "mysqltokentable") == 0)
                     memcpy(cfg.mysqltokentable, val, MAXVAL-1);

                  if(strcmp(key, "mysqlmisctable") == 0)
                     memcpy(cfg.mysqlmisctable, val, MAXVAL-1);

                  if(strcmp(key, "mysqlblackholetable") == 0)
                     memcpy(cfg.mysqlblackholetable, val, MAXVAL-1);

                  if(strcmp(key, "mysqlusertable") == 0)
                     memcpy(cfg.mysqlusertable, val, MAXVAL-1);

                  if(strcmp(key, "mysqltraininglogtable") == 0)
                     memcpy(cfg.mysqltraininglogtable, val, MAXVAL-1);

                  if(strcmp(key, "mysqlqueuetable") == 0)
                     memcpy(cfg.mysqlqueuetable, val, MAXVAL-1);

                  if(strcmp(key, "mysqlstattable") == 0)
                     memcpy(cfg.mysqlstattable, val, MAXVAL-1);

                  if(strcmp(key, "relocate_delay") == 0)
                     cfg.relocate_delay = atoi(val);

                  if(strcmp(key, "relocate_url") == 0)
                     memcpy(cfg.relocate_url, val, MAXVAL-1);

                  if(strcmp(key, "spamcgi_url") == 0)
                     memcpy(cfg.spamcgi_url, val, MAXVAL-1);

                  if(strcmp(key, "traincgi_url") == 0)
                     memcpy(cfg.traincgi_url, val, MAXVAL-1);

                  if(strcmp(key, "usercgi_url") == 0)
                     memcpy(cfg.usercgi_url, val, MAXVAL-1);

                  if(strcmp(key, "trainlogcgi_url") == 0)
                     memcpy(cfg.trainlogcgi_url, val, MAXVAL-1);

                  if(strcmp(key, "statcgi_url") == 0)
                     memcpy(cfg.statcgi_url, val, MAXVAL-1);

                  if(strcmp(key, "page_len") == 0)
                     cfg.page_len = atoi(val);

                  if(strcmp(key, "save_trained_emails") == 0)
                     cfg.save_trained_emails = atoi(val);

                  if(strcmp(key, "saved_ham_path") == 0)
                     memcpy(cfg.saved_ham_path, val, MAXVAL-1);

                  if(strcmp(key, "saved_spam_path") == 0)
                     memcpy(cfg.saved_spam_path, val, MAXVAL-1);

                  if(strcmp(key, "spam_mail_from") == 0)
                     memcpy(cfg.spam_mail_from, val, MAXVAL-1);

                  if(strcmp(key, "spam_smtp_addr") == 0)
                     memcpy(cfg.spam_smtp_addr, val, MAXVAL-1);

                  if(strcmp(key, "spam_smtp_port") == 0)
                     cfg.spam_smtp_port = atoi(val);


                  if(strcmp(key, "ldap_host") == 0)
                     memcpy(cfg.ldap_host, val, MAXVAL-1);

                  if(strcmp(key, "ldap_base") == 0)
                     memcpy(cfg.ldap_base, val, MAXVAL-1);

                  if(strcmp(key, "ldap_user") == 0)
                     memcpy(cfg.ldap_user, val, MAXVAL-1);

                  if(strcmp(key, "ldap_pwd") == 0)
                     memcpy(cfg.ldap_pwd, val, MAXVAL-1);

                  if(strcmp(key, "ldap_use_tls") == 0)
                     cfg.ldap_use_tls = atoi(val);


                  if(strcmp(key, "gocr") == 0)
                     memcpy(cfg.gocr, val, MAXVAL-1);

                  if(strcmp(key, "catdoc") == 0)
                     memcpy(cfg.catdoc, val, MAXVAL-1);


                  if(strcmp(key, "phishtankdb") == 0)
                     memcpy(cfg.phishtankdb, val, MAXVAL-1);

               }

            }
         }
         fclose(F);
      }
   }

   return cfg;
}
