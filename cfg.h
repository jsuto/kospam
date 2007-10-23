/*
 * cfg.h, 2007.10.23, SJ
 */

#define MAXVAL 128

struct __config {
   char hostid[MAXVAL];
   char pidfile[MAXVAL];

   char listen_addr[MAXVAL];
   int listen_port;

   char postfix_addr[MAXVAL];
   int postfix_port;

   char avg_addr[MAXVAL];
   int avg_port;

   char avast_addr[MAXVAL];
   int avast_port;

   char kav_socket[MAXVAL];

   char drweb_socket[MAXVAL];

   char clamd_socket[MAXVAL];

   int max_connections;
   int backlog;

   char chrootdir[MAXVAL];
   char workdir[MAXVAL];
   char queuedir[MAXVAL];
   char quarantine_dir[MAXVAL];
   char blackhole_path[MAXVAL];

   int verbosity;

   int session_timeout;

   char clapfemail[MAXVAL];
   char localpostmaster[MAXVAL];
   int silently_discard_infected_email;

   int use_antispam;

   char userprefdb[MAXVAL];

   char spam_subject_prefix[MAXVAL];

   char tokensfile[MAXVAL];

   int use_triplets;
   int use_pairs;
   int use_single_tokens;

   int enable_auto_white_list;

   int min_phrase_number;

   float rob_s;
   float rob_x;
   float esf_h;
   float esf_s;

   float exclusion_radius;

   unsigned long max_message_size_to_filter;

   char surbl_domain[MAXVAL];
   int rude_surbl;
   char rbl_domain[MAXVAL];

   char clapf_header_field[MAXVAL];
   char clapf_spam_header_field[MAXVAL];

   float max_ham_spamicity;

   float spam_overall_limit;
   float spaminess_oblivion_limit;
   float use_single_tokens_min_limit;
   float min_deviation_to_use_single_tokens;

   float spaminess_of_strange_language_stuff;
   float spaminess_of_too_much_spam_in_top15;
   float spaminess_of_blackholed_mail;
   float spaminess_of_text_and_base64;
   float spaminess_of_caught_by_surbl;
   float spaminess_of_embed_image;

   int use_all_the_most_interesting_tokens;

   float spam_ratio_in_top10;

   char skip_headers[MAXVAL];
   int num_of_skip_headers;

   int invalid_junk_limit;
   int invalid_junk_line;
   int invalid_hex_junk_limit;

   int penalize_images;
   int penalize_embed_images;
   int penalize_octet_stream;

   // training

   int training_mode;
   int group_type;
   int initial_1000_learning;

   int store_metadata;

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

   // sqlite3 stuff

   char sqlite3[MAXVAL];

   char mydbfile[MAXVAL];

   // Qcache

   char qcache_addr[MAXVAL];
   int qcache_port;
   char qcache_socket[MAXVAL];
   int qcache_update;

   // web interface

   int relocate_delay;
   char relocate_url[MAXVAL];
   char spamcgi_url[MAXVAL];
   char traincgi_url[MAXVAL];
   char trainlogcgi_url[MAXVAL];
   char usercgi_url[MAXVAL];
   char statcgi_url[MAXVAL];
   int page_len;

   char saved_ham_path[MAXVAL];
   char saved_spam_path[MAXVAL];
   int save_trained_emails;

   // spam quarantine

   char spam_mail_from[MAXVAL];
   char spam_quarantine_dir[MAXVAL];

   char spam_smtp_addr[MAXVAL];
   int spam_smtp_port;

   // ldap

   char ldap_host[MAXVAL];
   char ldap_base[MAXVAL];
   char ldap_user[MAXVAL];
   char ldap_pwd[MAXVAL];
   int ldap_use_tls;

   // ssl

   char ssl_cert_file[MAXVAL];
   char ssl_key_file[MAXVAL];

   // external applications
   char gocr[MAXVAL];
   char catdoc[MAXVAL];

   // phishing cdb
   char phishtankdb[MAXVAL];
};

struct __config read_config(char *configfile);

