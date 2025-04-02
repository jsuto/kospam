/*
 * cfg.h, SJ
 */

#ifndef _CFG_H
 #define _CFG_H

#include "config.h"

struct __config {
   char username[MAXVAL];

   char hostname[MAXVAL];

   int server_id;
   int server_mode;

   char pidfile[MAXVAL];

   char listen_addr[MAXVAL];

   char smtp_addr[MAXVAL];

   char clamd_addr[MAXVAL];
   int clamd_port;
   char clamd_socket[MAXVAL];
   int always_scan_message;

   int tls_enable;
   char pemfile[MAXVAL];
   char cipher_list[MAXVAL];

   char mynetwork[MAXVAL];

   char memcached_servers[MAXVAL];
   int memcached_ttl;

   int number_of_worker_processes;
   int max_requests_per_child;

   int backlog;

   char workdir[MAXVAL];
   char queuedir[MAXVAL];

   int history;

   int verbosity;
   char locale[MAXVAL];

   int helper_timeout;

   char spam_subject_prefix[MAXVAL];
   char possible_spam_subject_prefix[MAXVAL];

   int default_retention_days;

   // mysql stuff

   char mysqlhost[MAXVAL];
   int mysqlport;
   char mysqlsocket[MAXVAL];
   char mysqluser[MAXVAL];
   char mysqlpwd[MAXVAL];
   char mysqldb[MAXVAL];
   int mysql_connect_timeout;

   int update_tokens;
   int update_counters_to_memcached;
   int memcached_to_db_interval;

   int min_word_len;

   int enable_cjk;

   // training

   int training_mode;
   int group_type;
   int initial_1000_learning;

   int store_emails;
   int store_only_spam;

   int invalid_junk_limit;
   int invalid_junk_line;

   int penalize_images;
   int penalize_embed_images;
   int penalize_octet_stream;

   int max_number_of_recipients_in_ham;

   float max_ham_spamicity;

   float spam_overall_limit;
   float spaminess_oblivion_limit;
   float possible_spam_limit;

   char clapf_header_field[MAXVAL];
   char clapf_spam_header_field[MAXVAL];

   int max_message_size_to_filter;
   int max_number_of_tokens_to_filter;
   int max_line_len;

   char rbl_domain[MAXVAL];
   char surbl_domain[MAXVAL];

   float rob_s;
   float rob_x;
   float esf_h;
   float esf_s;

   float exclusion_radius;

   int silently_discard_infected_email;
   int deliver_infected_email;

   int surbl_condemns_the_message;

   char blackhole_email_list[MAXVAL];

   int replace_junk_characters;

   int message_from_a_zombie;

   int use_antispam;
   int use_antivirus;

   int log_subject;

   char our_signo[MAXVAL];
   char skipped_received_ips[MAXVAL];

   char maillog[MAXVAL];

   char mydomains[2*MAXVAL];
   int mydomains_from_outside_is_spam;

   int debug;
};


#endif /* _CFG_H */
