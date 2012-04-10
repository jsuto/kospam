/*
 * cfg.h, SJ
 */

#ifndef _CFG_H
 #define _CFG_H

#include "config.h"

struct __config {
   char username[MAXVAL];

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

   char memcached_servers[MAXVAL];
   int memcached_ttl;
   int update_counters_to_memcached;
   unsigned long memcached_to_db_interval;

   int max_connections;
   int backlog;

   char chrootdir[MAXVAL];
   char workdir[MAXVAL];
   char queuedir[MAXVAL];
   char quarantine_dir[MAXVAL];

   char our_signo[MAXVAL];

   int verbosity;
   int debug;
   char locale[MAXVAL];

   int session_timeout;

   char localpostmaster[MAXVAL];
   int silently_discard_infected_email;
   int deliver_infected_email;

   char blackhole_email_list[MAXVAL];

   int message_from_a_zombie;

   int use_antispam;

   char spam_subject_prefix[MAXVAL];
   char possible_spam_subject_prefix[MAXVAL];

   int enable_auto_white_list;

   float rob_s;
   float rob_x;
   float esf_h;
   float esf_s;

   float exclusion_radius;

   unsigned long max_message_size_to_filter;
   int max_number_of_tokens_to_filter;

   char rbl_domain[MAXVAL];
   char surbl_domain[MAXVAL];

   int rbl_condemns_the_message;
   int surbl_condemns_the_message;

   char clapf_header_field[MAXVAL];
   char clapf_spam_header_field[MAXVAL];
   char clapf_possible_spam_header_field[MAXVAL];

   int update_tokens;

   float max_ham_spamicity;

   float spam_overall_limit;
   float spaminess_oblivion_limit;
   float possible_spam_limit;

   float spaminess_of_strange_language_stuff;
   float spaminess_of_blackholed_mail;
   float spaminess_of_caught_by_rbl;
   float spaminess_of_caught_by_surbl;
   float spaminess_of_embed_image;

   int replace_junk_characters;
   int invalid_junk_limit;
   int invalid_junk_line;

   int penalize_images;
   int penalize_embed_images;
   int penalize_octet_stream;

   int max_number_of_recipients_in_ham;

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

   // PSql stuff

   char psqlhost[MAXVAL];
   int psqlport;
   char psqluser[MAXVAL];
   char psqlpwd[MAXVAL];
   char psqldb[MAXVAL];
   int psql_connect_timeout;

   // sqlite3 stuff

   char sqlite3[MAXVAL];
   char sqlite3_pragma[MAXVAL];

   char mydbfile[MAXVAL];

   char spam_smtp_addr[MAXVAL];
   int spam_smtp_port;

   // spamsum
   char sig_db[MAXVAL];

   char spamd_addr[MAXVAL];
   int spamd_port;
   char spamc_user[MAXVAL];


   // history
   char maillog[MAXVAL];
   char historydb[MAXVAL];
   char historypid[MAXVAL];

   char skipped_received_ips[2*MAXVAL];
   char mynetwork[2*MAXVAL];

   int days_to_retain_history_data;
   int days_to_retain_quarantine_data;

};


#endif /* _CFG_H */
