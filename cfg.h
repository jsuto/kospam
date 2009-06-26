/*
 * cfg.h, 2009.06.26, SJ
 */

#ifndef _CFG_H
 #define _CFG_H

#define MAXVAL 128

struct __config {
   char hostid[MAXVAL];
   char pidfile[MAXVAL];

   char listen_addr[MAXVAL];
   int listen_port;
   int listen_ssl_port;

   char postfix_addr[MAXVAL];
   int postfix_port;

   char delivery_agent[MAXVAL];

   char avg_addr[MAXVAL];
   int avg_port;

   char avast_addr[MAXVAL];
   int avast_port;

   char avast_home_cmd_line[MAXVAL];

   char kav_socket[MAXVAL];

   char drweb_socket[MAXVAL];

   char clamd_addr[MAXVAL];
   int clamd_port;
   char clamd_socket[MAXVAL];

   int max_connections;
   int backlog;

   char chrootdir[MAXVAL];
   char workdir[MAXVAL];
   char quarantine_dir[MAXVAL];
   char blackhole_path[MAXVAL];

   char our_signo[MAXVAL];
   char mydomains[MAXVAL];

   int verbosity;
   int debug;
   char locale[MAXVAL];

   int session_timeout;

   char localpostmaster[MAXVAL];
   int silently_discard_infected_email;
   int deliver_infected_email;

   char dha_trap_address_list[MAXVAL];
   char blackhole_email_list[MAXVAL];

   int use_antispam;

   char spam_subject_prefix[MAXVAL];

   int enable_auto_white_list;

   float rob_s;
   float rob_x;
   float esf_h;
   float esf_s;

   float exclusion_radius;

   unsigned long max_message_size_to_filter;

   char rbl_domain[MAXVAL];
   char surbl_domain[MAXVAL];

   int rbl_condemns_the_message;
   int surbl_condemns_the_message;

   char clapf_header_field[MAXVAL];
   char clapf_spam_header_field[MAXVAL];

   int update_tokens;

   float max_ham_spamicity;

   float spam_overall_limit;
   float spaminess_oblivion_limit;

   float spaminess_of_strange_language_stuff;
   float spaminess_of_too_much_spam_in_top15;
   float spaminess_of_blackholed_mail;
   float spaminess_of_text_and_base64;
   float spaminess_of_caught_by_surbl;
   float spaminess_of_embed_image;

   int replace_junk_characters;
   int invalid_junk_limit;
   int invalid_junk_line;

   int penalize_images;
   int penalize_embed_images;
   int penalize_octet_stream;

   // training

   int training_mode;
   int group_type;
   int initial_1000_learning;

   int store_metadata;
   int store_only_spam;

   int always_scan_message;

   // clamav stuff

   int use_libclamav_block_max_feature;
   int clamav_maxfile;
   long int clamav_max_archived_file_size;
   int clamav_max_recursion_level;
   int clamav_max_compress_ratio;
   int clamav_archive_mem_limit;
   int clamav_block_encrypted_archives;
   int clamav_use_phishing_db;

   // mysql stuff

   char mysqlhost[MAXVAL];
   int mysqlport;
   char mysqlsocket[MAXVAL];
   char mysqluser[MAXVAL];
   char mysqlpwd[MAXVAL];
   char mysqldb[MAXVAL];
   int mysql_connect_timeout;
   int mysql_enable_autoreconnect;

   // sqlite3 stuff

   char sqlite3[MAXVAL];
   char sqlite3_pragma[MAXVAL];

   char mydbfile[MAXVAL];

   // Qcache

   char qcache_addr[MAXVAL];
   int qcache_port;
   char qcache_socket[MAXVAL];
   int qcache_update;

   char spam_smtp_addr[MAXVAL];
   int spam_smtp_port;

   // ldap

   char ldap_host[MAXVAL];
   char ldap_base[MAXVAL];
   char ldap_user[MAXVAL];
   char ldap_pwd[MAXVAL];
   int ldap_use_tls;
   char email_address_attribute_name[MAXVAL];
   char email_alias_attribute_name[MAXVAL];

   // ssl

   char ssl_cert_file[MAXVAL];
   char ssl_key_file[MAXVAL];
   int use_ssl;

   // spamsum
   char sig_db[MAXVAL];

   char spamd_addr[MAXVAL];
   int spamd_port;
   char spamc_user[MAXVAL];

};

struct __config read_config(char *configfile);

#endif /* _CFG_H */
