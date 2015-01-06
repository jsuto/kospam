/*
 * cfg.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "cfg.h"
#include "misc.h"
#include "config.h"


int string_parser(char *src, char *target, int limit){
   snprintf(target, limit, "%s", src);

   return 0;
};

int multi_line_string_parser(char *src, char *target, int limit){
   if(strlen(src) > 0 && strlen(target) + strlen(src) + 3 < limit){
      strncat(target, src, limit);
      strncat(target, "\r\n", limit);

      return 0;
   }

   return 1;
};

int int_parser(char *src, int *target, int limit){
   *target = strtol(src, (char **) NULL, 10);

   return 0;
};


int float_parser(char *src, float *target, int limit){
   *target = strtof(src, (char **) NULL);

   return 0;
};


struct _parse_rule {
   char *name;
   char *type;
   int(*parser)(char*,void*,int);
   size_t offset;
   char *def_val;
   int limit;
};


/*
 * all known configuration items in order
 */

struct _parse_rule config_parse_rules[] =
{

   { "always_scan_message", "integer", (void*) int_parser, offsetof(struct __config, always_scan_message), "1", sizeof(int)},
   { "backlog", "integer", (void*) int_parser, offsetof(struct __config, backlog), "20", sizeof(int)},
   { "blackhole_email_list", "string", (void*) string_parser, offsetof(struct __config, blackhole_email_list), "", MAXVAL-1},
   { "cipher_list", "string", (void*) string_parser, offsetof(struct __config, cipher_list), "HIGH:MEDIUM", MAXVAL-1},
   { "clamd_addr", "string", (void*) string_parser, offsetof(struct __config, clamd_addr), "", MAXVAL-1},
   { "clamd_port", "integer", (void*) int_parser, offsetof(struct __config, clamd_port), "0", sizeof(int)},
   { "clamd_socket", "string", (void*) string_parser, offsetof(struct __config, clamd_socket), CLAMD_SOCKET, MAXVAL-1},
   { "clapf_header_field", "string", (void*) string_parser, offsetof(struct __config, clapf_header_field), SPAMINESS_HEADER_FIELD, MAXVAL-1},
   { "clapf_spam_header_field", "multi_line_string", (void*) multi_line_string_parser, offsetof(struct __config, clapf_spam_header_field), SPAM_HEADER_FIELD, MAXVAL-1},
   { "debug", "integer", (void*) int_parser, offsetof(struct __config, debug), "0", sizeof(int)},
   { "default_retention_days", "integer", (void*) int_parser, offsetof(struct __config, default_retention_days), "2557", sizeof(int)},
   { "enable_cjk", "integer", (void*) int_parser, offsetof(struct __config, enable_cjk), "0", sizeof(int)},
   { "esf_h", "float", (void*) float_parser, offsetof(struct __config, esf_h), "1.0", sizeof(float)},
   { "esf_s", "float", (void*) float_parser, offsetof(struct __config, esf_s), "1.0", sizeof(float)},
   { "exclusion_radius", "float", (void*) float_parser, offsetof(struct __config, exclusion_radius), "0.375", sizeof(float)},
   { "group_type", "integer", (void*) int_parser, offsetof(struct __config, group_type), "0", sizeof(int)},
   { "helper_timeout", "integer", (void*) int_parser, offsetof(struct __config, helper_timeout), "20", sizeof(int)},
   { "history", "integer", (void*) int_parser, offsetof(struct __config, history), "0", sizeof(int)},
   { "hostid", "string", (void*) string_parser, offsetof(struct __config, hostid), HOSTID, MAXVAL-1},
   { "listen_addr", "string", (void*) string_parser, offsetof(struct __config, listen_addr), "127.0.0.1", MAXVAL-1},
   { "listen_port", "integer", (void*) int_parser, offsetof(struct __config, listen_port), "10025", sizeof(int)},
   { "locale", "string", (void*) string_parser, offsetof(struct __config, locale), "", MAXVAL-1},
   { "maillog", "string", (void*) string_parser, offsetof(struct __config, maillog), "", MAXVAL-1},
   { "max_ham_spamicity", "float", (void*) float_parser, offsetof(struct __config, max_ham_spamicity), "0.45", sizeof(float)},
   { "max_message_size_to_filter", "integer", (void*) int_parser, offsetof(struct __config, max_message_size_to_filter), "65535", sizeof(int)},
   { "max_number_of_recipients_in_ham", "integer", (void*) int_parser, offsetof(struct __config, max_number_of_recipients_in_ham), "9999", sizeof(int)},
   { "max_number_of_tokens_to_filter", "integer", (void*) int_parser, offsetof(struct __config, max_number_of_tokens_to_filter), "2000", sizeof(int)},
   { "max_requests_per_child", "integer", (void*) int_parser, offsetof(struct __config, max_requests_per_child), "1000", sizeof(int)},
   { "memcached_servers", "string", (void*) string_parser, offsetof(struct __config, memcached_servers), "127.0.0.1", MAXVAL-1},
   { "memcached_to_db_interval", "integer", (void*) int_parser, offsetof(struct __config, memcached_to_db_interval), "900", sizeof(int)},
   { "memcached_ttl", "integer", (void*) int_parser, offsetof(struct __config, memcached_ttl), "86400", sizeof(int)},
   { "message_from_a_zombie", "integer", (void*) int_parser, offsetof(struct __config, message_from_a_zombie), "0", sizeof(int)},
   { "min_word_len", "integer", (void*) int_parser, offsetof(struct __config, min_word_len), "1", sizeof(int)},
   { "mydomains", "string", (void*) string_parser, offsetof(struct __config, mydomains), "", MAXVAL-1},
   { "mydomains_from_outside_is_spam", "integer", (void*) int_parser, offsetof(struct __config, mydomains_from_outside_is_spam), "0", sizeof(int)},
   { "mynetwork", "string", (void*) string_parser, offsetof(struct __config, mynetwork), "", MAXVAL-1},
   { "mysqlhost", "string", (void*) string_parser, offsetof(struct __config, mysqlhost), "", MAXVAL-1},
   { "mysqlport", "integer", (void*) int_parser, offsetof(struct __config, mysqlport), "", sizeof(int)},
   { "mysqlsocket", "string", (void*) string_parser, offsetof(struct __config, mysqlsocket), "/tmp/mysql.sock", MAXVAL-1},
   { "mysqluser", "string", (void*) string_parser, offsetof(struct __config, mysqluser), "piler", MAXVAL-1},
   { "mysqlpwd", "string", (void*) string_parser, offsetof(struct __config, mysqlpwd), "", MAXVAL-1},
   { "mysqldb", "string", (void*) string_parser, offsetof(struct __config, mysqldb), "piler", MAXVAL-1},
   { "mysql_connect_timeout", "integer", (void*) int_parser, offsetof(struct __config, mysql_connect_timeout), "2", sizeof(int)},
   { "number_of_worker_processes", "integer", (void*) int_parser, offsetof(struct __config, number_of_worker_processes), "10", sizeof(int)},
   { "our_signo", "string", (void*) string_parser, offsetof(struct __config, our_signo), "", MAXVAL-1},
   { "pemfile", "string", (void*) string_parser, offsetof(struct __config, pemfile), "", MAXVAL-1},
   { "penalize_embed_images", "integer", (void*) int_parser, offsetof(struct __config, penalize_embed_images), "0", sizeof(int)},
   { "penalize_images", "integer", (void*) int_parser, offsetof(struct __config, penalize_images), "0", sizeof(int)},
   { "penalize_octet_stream", "integer", (void*) int_parser, offsetof(struct __config, penalize_octet_stream), "0", sizeof(int)},
   { "pidfile", "string", (void*) string_parser, offsetof(struct __config, pidfile), PIDFILE, MAXVAL-1},
   { "possible_spam_limit", "float", (void*) float_parser, offsetof(struct __config, possible_spam_limit), "0.8", sizeof(float)},
   { "possible_spam_subject_prefix", "string", (void*) string_parser, offsetof(struct __config, possible_spam_subject_prefix), "", MAXVAL-1},
   { "queuedir", "string", (void*) string_parser, offsetof(struct __config, queuedir), QUEUE_DIR, MAXVAL-1},
   { "replace_junk_characters", "integer", (void*) int_parser, offsetof(struct __config, replace_junk_characters), "1", sizeof(int)},
   { "rob_s", "float", (void*) float_parser, offsetof(struct __config, rob_s), "1.0", sizeof(float)},
   { "rob_x", "float", (void*) float_parser, offsetof(struct __config, rob_x), "0.52", sizeof(float)},
   { "server_id", "integer", (void*) int_parser, offsetof(struct __config, server_id), "0", sizeof(int)},
   { "server_mode", "integer", (void*) int_parser, offsetof(struct __config, server_mode), "1", sizeof(int)},
   { "silently_discard_infected_email", "integer", (void*) int_parser, offsetof(struct __config, silently_discard_infected_email), "1", sizeof(int)},
   { "skipped_received_ips", "string", (void*) string_parser, offsetof(struct __config, skipped_received_ips), "", MAXVAL-1},
   { "smtp_addr", "string", (void*) string_parser, offsetof(struct __config, smtp_addr), "127.0.0.1", MAXVAL-1},
   { "smtp_port", "integer", (void*) int_parser, offsetof(struct __config, smtp_port), "10026", sizeof(int)},
   { "spaminess_oblivion_limit", "float", (void*) float_parser, offsetof(struct __config, spaminess_oblivion_limit), "1.01", sizeof(float)},
   { "spam_overall_limit", "float", (void*) float_parser, offsetof(struct __config, spam_overall_limit), "0.92", sizeof(float)},
   { "spam_subject_prefix", "string", (void*) string_parser, offsetof(struct __config, spam_subject_prefix), "", MAXVAL-1},
   { "store_emails", "integer", (void*) int_parser, offsetof(struct __config, store_emails), "1", sizeof(int)},
   { "store_only_spam", "integer", (void*) int_parser, offsetof(struct __config, store_only_spam), "1", sizeof(int)},
   { "surbl_domain", "string", (void*) string_parser, offsetof(struct __config, surbl_domain), "", MAXVAL-1},
   { "surbl_condemns_the_message", "integer", (void*) int_parser, offsetof(struct __config, surbl_condemns_the_message), "0", sizeof(int)},
   { "tls_enable", "integer", (void*) int_parser, offsetof(struct __config, tls_enable), "0", sizeof(int)},
   { "training_mode", "integer", (void*) int_parser, offsetof(struct __config, training_mode), "0", sizeof(int)},
   { "update_counters_to_memcached", "integer", (void*) int_parser, offsetof(struct __config, update_counters_to_memcached), "0", sizeof(int)},
   { "update_tokens", "integer", (void*) int_parser, offsetof(struct __config, update_tokens), "1", sizeof(int)},
   { "username", "string", (void*) string_parser, offsetof(struct __config, username), "clapf", MAXVAL-1},
   { "use_antispam", "integer", (void*) int_parser, offsetof(struct __config, use_antispam), "1", sizeof(int)},
   { "use_antivirus", "integer", (void*) int_parser, offsetof(struct __config, use_antivirus), "1", sizeof(int)},
   { "verbosity", "integer", (void*) int_parser, offsetof(struct __config, verbosity), "1", sizeof(int)},
   { "workdir", "string", (void*) string_parser, offsetof(struct __config, workdir), WORK_DIR, MAXVAL-1},

   {NULL, NULL, NULL, 0, 0}
};


/*
 * parse configfile
 */

int parse_config_file(char *configfile, struct __config *target_cfg, struct _parse_rule *rules){
   char line[MAXVAL], *chpos;
   FILE *f;

   if(!configfile) return -1;

   f = fopen(configfile, "r");
   if(!f) return -1;

   while(fgets(&line[0], sizeof(line)-1, f)){
      if(line[0] == ';' || line[0] == '#') continue;

      chpos = strchr(line, '=');

      if(chpos){
         trim_buffer(chpos+1);
         *chpos = '\0';
         int i = 0;

         while(rules[i].name){
            if(!strcmp(line, rules[i].name)) {
               if(rules[i].parser(chpos+1, (char*)target_cfg + rules[i].offset, rules[i].limit)){
                  printf("failed to parse %s: \"%s\"\n", line, chpos+1);
               }
               break;
            }				

            i++;
         }

         if(!rules[i].name) syslog(LOG_PRIORITY, "unknown key: \"%s\"", line);
      }
   }

   fclose(f);

   return 0;
}


int load_default_config(struct __config *cfg, struct _parse_rule *rules){
   int i=0;

   while(rules[i].name){
      rules[i].parser(rules[i].def_val, (char*)cfg + rules[i].offset, rules[i].limit);
      i++;
   }

   return 0;
}


/*
 * read configuration file variables
 */

struct __config read_config(char *configfile){
   struct __config cfg;

   /* reset config structure and fill it with defaults */

   memset((char *)&cfg, 0, sizeof(struct __config));

   load_default_config(&cfg, config_parse_rules);


   /* parse the config file */

   if(parse_config_file(configfile, &cfg, config_parse_rules) == -1) printf("error parsing the configfile: %s\n", configfile);

   cfg.hostid_len = strlen(cfg.hostid);

   return cfg;
}


/*
 * print a single configuration item as key=value
 */

void print_config_item(struct __config *cfg, struct _parse_rule *rules, int i){
   int j;
   float f;
   char *p, buf[MAXVAL];

   p = (char*)cfg + rules[i].offset;

   if(strcmp(rules[i].type, "integer") == 0){
      memcpy((char*)&j, p, sizeof(int));
      printf("%s=%d\n", rules[i].name, j);
   }
   else if(strcmp(rules[i].type, "float") == 0){
      memcpy((char*)&f, p, sizeof(float));
      printf("%s=%.4f\n", rules[i].name, f);
   }
   else if(strcmp(rules[i].type, "multi_line_string") == 0){
      j = 0;
      do {
         p = split_str(p, "\r\n", buf, sizeof(buf)-1);
         if(p || !j) printf("%s=%s\n", rules[i].name, buf);
         j++;
      } while(p);
   }
   else {
      trim_buffer(p);
      printf("%s=%s\n", rules[i].name, p);
   }
 
}


/*
 * print all known configuration items
 */

void print_config_all(struct __config *cfg, char *key){
   int i=0;
   struct _parse_rule *rules;

   rules = &config_parse_rules[0];

   while(rules[i].name){
      if(key == NULL){
         print_config_item(cfg, rules, i);
      }
      else {
         if(strcmp(key, rules[i].name) == 0)
            print_config_item(cfg, rules, i);
      }

      i++;
   }
}


/*
 * print all configuration items found in configfile
 */

void print_config(char *configfile, struct __config *cfg){
   FILE *f;
   char line[MAXVAL], *chpos, previtem[MAXVAL];
   struct _parse_rule *rules;


   if(!configfile) return;

   f = fopen(configfile, "r");
   if(!f) return;

   rules = &config_parse_rules[0];

   memset(previtem, 0, sizeof(previtem));

   while(fgets(&line[0], sizeof(line)-1, f)){
      if(line[0] == ';' || line[0] == '#') continue;

      chpos = strchr(line, '=');

      if(chpos){
         trim_buffer(chpos+1);
         *chpos = '\0';
         int i = 0;

         while(rules[i].name){
            if(strcmp(line, rules[i].name) == 0) {
               if(strcmp(line, previtem)) print_config_item(cfg, rules, i);

               snprintf(previtem, sizeof(previtem)-1, "%s", line);
               break;
            }

            i++;
         }
 
         if(!rules[i].name) printf("unknown key: \"%s\" \n", line);
      }
   }

   fclose(f);
}


