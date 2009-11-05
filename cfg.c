/*
 * cfg.c, 2009.11.05, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfg.h"
#include "misc.h"
#include "config.h"


/*
 * read configuration file variables
 */

struct __config read_config(char *configfile){
   struct __config cfg;
   char *p, buf[MAXBUFSIZE], key[MAXVAL], val[MAXVAL];
   FILE *F;

   /* reset config structure and fill it with defaults */

   memset((char *)&cfg, 0, sizeof(struct __config));

   cfg.verbosity = 1;
   cfg.debug = 0;

   strncpy(cfg.hostid, HOSTID, MAXVAL-1);

   strncpy(cfg.workdir, WORK_DIR, MAXVAL-1);
   strncpy(cfg.pidfile, PIDFILE, MAXVAL-1);


   strncpy(cfg.listen_addr, "127.0.0.1", MAXVAL-1);
   cfg.listen_port = 10025;

   strncpy(cfg.postfix_addr, "127.0.0.1", MAXVAL-1);
   cfg.postfix_port = 10026;

   strncpy(cfg.spam_smtp_addr, "127.0.0.1", MAXVAL-1);
   cfg.spam_smtp_port = 10026;


   strncpy(cfg.delivery_agent, "/usr/sbin/sendmail -oi -f", MAXVAL-1);

   strncpy(cfg.avast_addr, "127.0.0.1", MAXVAL-1);
   cfg.avast_port = 5036;

   strncpy(cfg.avast_home_cmd_line, "/usr/bin/avast", MAXVAL-1);

   strncpy(cfg.kav_socket, KAV_SOCKET, MAXVAL-1);
   strncpy(cfg.drweb_socket, DRWEB_SOCKET, MAXVAL-1);
   strncpy(cfg.clamd_socket, CLAMD_SOCKET, MAXVAL-1);

   cfg.max_connections = 30;
   cfg.backlog = 20;
   cfg.session_timeout = 420;


   cfg.always_scan_message = 1;
   cfg.silently_discard_infected_email = 1;
   cfg.deliver_infected_email = 0;

   cfg.message_from_a_zombie = 0;

   strncpy(cfg.memcached_servers, "127.0.0.1", MAXVAL-1);
   cfg.use_antispam = 1;

   cfg.enable_auto_white_list = 1;


   cfg.rob_s = 1.0;
   cfg.rob_x = 0.52;
   cfg.esf_h = 1.0;
   cfg.esf_s = 1.0;

   cfg.exclusion_radius = 0.375;

   cfg.max_message_size_to_filter = 65535;
   cfg.max_number_of_tokens_to_filter = 2000;


   cfg.penalize_images = 0;
   cfg.penalize_embed_images = 0;
   cfg.penalize_octet_stream = 0;

   //strncpy(cfg.surbl_domain, "multi.surbl.org", MAXVAL-1);
   //strncpy(cfg.rbl_domain, "zen.spamhaus.org", MAXVAL-1);

   strncpy(cfg.clapf_header_field, SPAMINESS_HEADER_FIELD, MAXVAL-1);

   //strncpy(cfg.spam_subject_prefix, "[sp@m]", MAXVAL-1);

   cfg.spam_overall_limit = 0.92;
   cfg.spaminess_oblivion_limit = 1.01;
   cfg.possible_spam_limit = 0.8;
   cfg.max_ham_spamicity = 0.45;


   cfg.spaminess_of_strange_language_stuff = 0.9876;
   cfg.spaminess_of_blackholed_mail = 0.9995;
   cfg.spaminess_of_text_and_base64 = 0;
   cfg.spaminess_of_caught_by_surbl = 0.9997;
   cfg.spaminess_of_embed_image = 0.9994;

   cfg.group_type = 1;
   cfg.training_mode = 0;

   cfg.initial_1000_learning = 0;

   cfg.update_tokens = 1;

   cfg.store_metadata = 1;
   cfg.store_only_spam = 0;

   cfg.replace_junk_characters = 1;
   cfg.invalid_junk_limit = 5;
   cfg.invalid_junk_line = 1;


   cfg.use_libclamav_block_max_feature = 1;
   cfg.clamav_maxfile = 100;
   cfg.clamav_max_archived_file_size = 31457280;
   cfg.clamav_max_recursion_level = 5;
   cfg.clamav_max_compress_ratio = 200;
   cfg.clamav_archive_mem_limit = 0;
   cfg.clamav_block_encrypted_archives = 1;
   cfg.clamav_use_phishing_db = 1;


   strncpy(cfg.mysqlsocket, "/tmp/mysql.sock", MAXVAL-1);
   strncpy(cfg.mysqluser, "clapf", MAXVAL-1);
   //cfg.mysqlpwd, "");
   strncpy(cfg.mysqldb, "clapf", MAXVAL-1);
   cfg.mysql_connect_timeout = 2;
   cfg.mysql_enable_autoreconnect = 0;


   snprintf(cfg.sqlite3, MAXVAL-1, "%s/tokens.sdb", USER_DATA_DIR);
   strncpy(cfg.sqlite3_pragma, "PRAGMA synchronous = OFF", MAXVAL-1);

   snprintf(cfg.mydbfile, MAXVAL-1, "%s/tokens.mydb", USER_DATA_DIR);

   strncpy(cfg.ldap_host, "127.0.0.1", MAXVAL-1);
   strncpy(cfg.ldap_base, "dc=yourdomain,dc=com", MAXVAL-1);
   cfg.ldap_use_tls = 0;

   strncpy(cfg.email_address_attribute_name, "mail", MAXVAL-1);
   strncpy(cfg.email_alias_attribute_name, "mailAlternateAddress", MAXVAL-1);

   strncpy(cfg.spamd_addr, "127.0.0.1", MAXVAL-1);
   cfg.spamd_port = 783;
   strncpy(cfg.spamc_user, "spamc", MAXVAL-1);



   /* parse the config file */

   if(configfile){
      F = fopen(configfile, "r");
      if(F){
         while(fgets(buf, MAXBUFSIZE, F)){
            if(buf[0] != ';' && buf[0] != '#'){
               memset(key, 0, MAXVAL);
               memset(val, 0, MAXVAL);

               p = split(buf, '=', key, MAXVAL-1);
               p = split(p, '\n', val, MAXVAL-1);


               if(p){
                  if(strcmp(key, "hostid") == 0)
                     memcpy(cfg.hostid, val, MAXVAL-1);

                  if(strcmp(key, "listen_addr") == 0)
                     memcpy(cfg.listen_addr, val, MAXVAL-1);

                  if(strcmp(key, "listen_port") == 0)
                     cfg.listen_port = atoi(val);

                  if(strcmp(key, "listen_ssl_port") == 0)
                     cfg.listen_ssl_port = atoi(val);

                  if(strcmp(key, "postfix_addr") == 0)
                     memcpy(cfg.postfix_addr, val, MAXVAL-1);

                  if(strcmp(key, "postfix_port") == 0)
                     cfg.postfix_port = atoi(val);

                  if(strcmp(key, "delivery_agent") == 0)
                     memcpy(cfg.delivery_agent, val, MAXVAL-1);

                  if(strcmp(key, "avg_addr") == 0)
                     memcpy(cfg.avg_addr, val, MAXVAL-1);

                  if(strcmp(key, "avg_port") == 0)
                     cfg.avg_port = atoi(val);

                  if(strcmp(key, "avast_addr") == 0)
                     memcpy(cfg.avast_addr, val, MAXVAL-1);

                  if(strcmp(key, "avast_port") == 0)
                     cfg.avast_port = atoi(val);

                  if(strcmp(key, "avast_home_cmd_line") == 0)
                     memcpy(cfg.avast_home_cmd_line, val, MAXVAL-1);

                  if(strcmp(key, "kav_socket") == 0)
                     memcpy(cfg.kav_socket, val, MAXVAL-1);

                  if(strcmp(key, "drweb_socket") == 0)
                     memcpy(cfg.drweb_socket, val, MAXVAL-1);

                  if(strcmp(key, "clamd_addr") == 0)
                     memcpy(cfg.clamd_addr, val, MAXVAL-1);

                  if(strcmp(key, "clamd_port") == 0)
                     cfg.clamd_port = atoi(val);

                  if(strcmp(key, "clamd_socket") == 0)
                     memcpy(cfg.clamd_socket, val, MAXVAL-1);

                  if(strcmp(key, "memcached_servers") == 0)
                     memcpy(cfg.memcached_servers, val, MAXVAL-1);

                  if(strcmp(key, "always_scan_message") == 0)
                     cfg.always_scan_message = atoi(val);

                  if(strcmp(key, "max_connections") == 0)
                     cfg.max_connections = atoi(val);

                  if(strcmp(key, "backlog") == 0)
                     cfg.backlog = atoi(val);

                  if(strcmp(key, "chrootdir") == 0)
                     memcpy(cfg.chrootdir, val, MAXVAL-1);

                  if(strcmp(key, "workdir") == 0)
                     memcpy(cfg.workdir, val, MAXVAL-1);

                  if(strcmp(key, "quarantine_dir") == 0)
                     memcpy(cfg.quarantine_dir, val, MAXVAL-1);

                  if(strcmp(key, "our_signo") == 0)
                     memcpy(cfg.our_signo, val, MAXVAL-1);

                  if(strcmp(key, "mydomains") == 0)
                     memcpy(cfg.mydomains, val, MAXVAL-1);
 
                  if(strcmp(key, "verbosity") == 0)
                     cfg.verbosity = atoi(val);

                  if(strcmp(key, "debug") == 0)
                     cfg.debug = atoi(val);

                  if(strcmp(key, "locale") == 0)
                     memcpy(cfg.locale, val, MAXVAL-1);

                  if(strcmp(key, "session_timeout") == 0)
                     cfg.session_timeout = atoi(val);

                  if(strcmp(key, "localpostmaster") == 0)
                     memcpy(cfg.localpostmaster, val, MAXVAL-1);

                  if(strcmp(key, "silently_discard_infected_email") == 0)
                     cfg.silently_discard_infected_email = atoi(val);

                  if(strcmp(key, "deliver_infected_email") == 0)
                     cfg.deliver_infected_email = atoi(val);

                  if(strcmp(key, "blackhole_email_list") == 0)
                     memcpy(cfg.blackhole_email_list, val, MAXVAL-1);

                  if(strcmp(key, "message_from_a_zombie") == 0)
                     cfg.message_from_a_zombie = atoi(val);

                  if(strcmp(key, "use_antispam") == 0)
                     cfg.use_antispam = atoi(val);


                  if(strcmp(key, "spam_subject_prefix") == 0)
                     memcpy(cfg.spam_subject_prefix, val, MAXVAL-1);

                  if(strcmp(key, "possible_spam_subject_prefix") == 0)
                     memcpy(cfg.possible_spam_subject_prefix, val, MAXVAL-1);

                  if(strcmp(key, "enable_auto_white_list") == 0)
                     cfg.enable_auto_white_list = atoi(val);

                  if(strcmp(key, "rob_s") == 0)
                     cfg.rob_s = atof(val);

                  if(strcmp(key, "rob_x") == 0)
                     cfg.rob_x = atof(val);

                  if(strcmp(key, "esf_h") == 0)
                     cfg.esf_h = atof(val);

                  if(strcmp(key, "esf_s") == 0)
                     cfg.esf_s = atof(val);

                  if(strcmp(key, "exclusion_radius") == 0)
                     cfg.exclusion_radius = atof(val);

                  if(strcmp(key, "max_message_size_to_filter") == 0)
                     cfg.max_message_size_to_filter = atol(val);

                  if(strcmp(key, "max_number_of_tokens_to_filter") == 0)
                     cfg.max_number_of_tokens_to_filter = atoi(val);

                  if(strcmp(key, "surbl_domain") == 0)
                     memcpy(cfg.surbl_domain, val, MAXVAL-1);

                  if(strcmp(key, "rbl_domain") == 0)
                     memcpy(cfg.rbl_domain, val, MAXVAL-1);

                  if(strcmp(key, "rbl_condemns_the_message") == 0)
                     cfg.rbl_condemns_the_message = atoi(val);

                  if(strcmp(key, "surbl_condemns_the_message") == 0)
                     cfg.surbl_condemns_the_message = atoi(val);

                  if(strcmp(key, "clapf_header_field") == 0)
                     memcpy(cfg.clapf_header_field, val, MAXVAL-1);

                  /* allow adding multiple header lines in case of spam, credits: Mariano, 2006.08.14 */

                  if(strcmp(key, "clapf_spam_header_field") == 0){
                     strncat(cfg.clapf_spam_header_field, val, MAXVAL-1);
                     strncat(cfg.clapf_spam_header_field, "\r\n", MAXVAL-1);
                  }

                  if(strcmp(key, "clapf_possible_spam_header_field") == 0)
                     memcpy(cfg.clapf_possible_spam_header_field, val, MAXVAL-1);

                  if(strcmp(key, "spam_overall_limit") == 0)
                     cfg.spam_overall_limit = atof(val);

                  if(strcmp(key, "update_tokens") == 0)
                     cfg.update_tokens = atoi(val);

                  if(strcmp(key, "spaminess_oblivion_limit") == 0)
                     cfg.spaminess_oblivion_limit = atof(val);

                  if(strcmp(key, "possible_spam_limit") == 0)
                     cfg.possible_spam_limit = atof(val);

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

                  if(strcmp(key, "replace_junk_characters") == 0)
                     cfg.replace_junk_characters = atoi(val);

                  if(strcmp(key, "invalid_junk_limit") == 0)
                     cfg.invalid_junk_limit = atoi(val);

                  if(strcmp(key, "invalid_junk_line") == 0)
                     cfg.invalid_junk_line = atoi(val);

                  if(strcmp(key, "max_ham_spamicity") == 0)
                     cfg.max_ham_spamicity = atof(val);

                  if(strcmp(key, "penalize_images") == 0)
                     cfg.penalize_images = atoi(val);

                  if(strcmp(key, "penalize_embed_images") == 0)
                     cfg.penalize_embed_images = atoi(val);

                  if(strcmp(key, "penalize_octet_stream") == 0)
                     cfg.penalize_octet_stream = atoi(val);

                  if(strcmp(key, "training_mode") == 0)
                     cfg.training_mode = atoi(val);

                  if(strcmp(key, "group_type") == 0)
                     cfg.group_type = atoi(val);

                  if(strcmp(key, "initial_1000_learning") == 0)
                     cfg.initial_1000_learning = atoi(val);

                  if(strcmp(key, "store_metadata") == 0)
                     cfg.store_metadata = atoi(val);

                  if(strcmp(key, "store_only_spam") == 0)
                     cfg.store_only_spam = atoi(val);

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

                  if(strcmp(key, "clamav_block_encrypted_archives") == 0)
                     cfg.clamav_block_encrypted_archives = atoi(val);

                  if(strcmp(key, "clamav_use_phishing_db") == 0)
                     cfg.clamav_use_phishing_db = atoi(val);

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

                  if(strcmp(key, "mysql_connect_timeout") == 0)
                     cfg.mysql_connect_timeout = atoi(val);

                  if(strcmp(key, "mysql_enable_autoreconnect") == 0)
                     cfg.mysql_enable_autoreconnect = atoi(val);

                  if(strcmp(key, "sqlite3") == 0)
                     memcpy(cfg.sqlite3, val, MAXVAL-1);

                  if(strcmp(key, "sqlite3_pragma") == 0)
                     memcpy(cfg.sqlite3_pragma, val, MAXVAL-1);

                  if(strcmp(key, "mydbfile") == 0)
                     memcpy(cfg.mydbfile, val, MAXVAL-1);

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

                  if(strcmp(key, "email_address_attribute_name") == 0)
                     memcpy(cfg.email_address_attribute_name, val, MAXVAL-1);

                  if(strcmp(key, "email_alias_attribute_name") == 0)
                     memcpy(cfg.email_alias_attribute_name, val, MAXVAL-1);

                  if(strcmp(key, "ssl_cert_file") == 0)
                     memcpy(cfg.ssl_cert_file, val, MAXVAL-1);

                  if(strcmp(key, "ssl_key_file") == 0)
                     memcpy(cfg.ssl_key_file, val, MAXVAL-1);



                  if(strcmp(key, "sig_db") == 0)
                     memcpy(cfg.sig_db, val, MAXVAL-1);

                  if(strcmp(key, "pidfile") == 0)
                     memcpy(cfg.pidfile, val, MAXVAL-1);

                  if(strcmp(key, "spamd_addr") == 0)
                     memcpy(cfg.spamd_addr, val, MAXVAL-1);

                  if(strcmp(key, "spamd_port") == 0)
                     cfg.spamd_port = atoi(val);

                  if(strcmp(key, "spamc_user") == 0)
                     memcpy(cfg.spamc_user, val, MAXVAL-1);

               }

            }
         }
         fclose(F);
      }
   }

   return cfg;
}
