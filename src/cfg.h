/*
 * cfg.h, SJ
 */

#ifndef _CFG_H
 #define _CFG_H

#include "config.h"

struct config {
   char username[MAXVAL];

   char hostname[MAXVAL];

   char pidfile[MAXVAL];

   char listen_addr[MAXVAL];

   char smtp_addr[MAXVAL];

   int tls_enable;
   char pemfile[MAXVAL];

   char mynetwork[MAXVAL];

   int number_of_worker_processes;
   int max_requests_per_child;

   char workdir[MAXVAL];
   char quarantinedir[MAXVAL];

   int history;

   int verbosity;
   char locale[MAXVAL];

   char spam_subject_prefix[MAXVAL];
   char possible_spam_subject_prefix[MAXVAL];

   // mysql stuff

   char mysqlhost[MAXVAL];
   int mysqlport;
   char mysqlsocket[MAXVAL];
   char mysqluser[MAXVAL];
   char mysqlpwd[MAXVAL];
   char mysqldb[MAXVAL];
   int mysql_connect_timeout;

   int update_tokens;

   int min_word_len;
   int max_word_len;

   // training

   int training_mode;

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

   int message_from_a_zombie;

   int log_subject;

   char our_signo[MAXVAL];
   char skipped_received_ips[MAXVAL];
   int received_lines_to_skip;

   char enable_xforward[MAXVAL];

   int debug;
};


#endif /* _CFG_H */
