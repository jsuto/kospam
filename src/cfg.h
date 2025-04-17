/*
 * cfg.h, SJ
 */

#ifndef _CFG_H
 #define _CFG_H

#include "config.h"

struct config {
   char blackhole_email_list[MAXVAL];
   int debug;
   char enable_xforward[MAXVAL];
   float esf_h;
   float esf_s;
   float exclusion_radius;
   int history;
   char hostname[MAXVAL];
   char kospam_header_field[MAXVAL];
   char kospam_spam_header_field[MAXVAL];
   char listen_addr[MAXVAL];
   char locale[MAXVAL];
   int log_subject;
   float max_ham_spamicity;
   int max_line_len;
   int max_message_size_to_filter;
   int max_requests_per_child;
   int max_word_len;
   int message_from_a_zombie;
   int min_word_len;
   char mynetwork[MAXVAL];
   char mysqlhost[MAXVAL];
   int mysqlport;
   char mysqlsocket[MAXVAL];
   char mysqluser[MAXVAL];
   char mysqlpwd[MAXVAL];
   char mysqldb[MAXVAL];
   int mysql_connect_timeout;
   int number_of_worker_processes;
   char our_signo[MAXVAL];
   int penalize_embed_images;
   int penalize_images;
   int penalize_octet_stream;
   char pemfile[MAXVAL];
   char pidfile[MAXVAL];
   float possible_spam_limit;
   char possible_spam_subject_prefix[MAXVAL];
   char quarantinedir[MAXVAL];
   int received_lines_to_skip;
   float rob_s;
   float rob_x;
   char skipped_received_ips[MAXVAL];
   char smtp_addr[MAXVAL];
   float spam_overall_limit;
   float spaminess_oblivion_limit;
   char spam_subject_prefix[MAXVAL];
   int update_tokens;
   char username[MAXVAL];
   int verbosity;
   char workdir[MAXVAL];
};


#endif /* _CFG_H */
