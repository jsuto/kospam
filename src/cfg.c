/*
 * cfg.c, SJ
 */

#include <kospam.h>
#include "cfg.h"
#include "config.h"


int string_parser(char *src, char *target, int limit){
   snprintf(target, limit, "%s", src);

   return 0;
};

int int_parser(char *src, int *target){
   *target = strtol(src, (char **) NULL, 10);

   return 0;
};


int float_parser(char *src, float *target){
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

   { "blackhole_email_list", "string", (void*) string_parser, offsetof(struct config, blackhole_email_list), "", MAXVAL-1},
   { "clapf_header_field", "string", (void*) string_parser, offsetof(struct config, clapf_header_field), SPAMINESS_HEADER_FIELD, MAXVAL-1},
   { "clapf_spam_header_field", "string", (void*) string_parser, offsetof(struct config, clapf_spam_header_field), "", MAXVAL-1},
   { "debug", "integer", (void*) int_parser, offsetof(struct config, debug), "0", sizeof(int)},
   { "enable_xforward", "string", (void*) string_parser, offsetof(struct config, enable_xforward), "false", MAXVAL-1},
   { "esf_h", "float", (void*) float_parser, offsetof(struct config, esf_h), "1.0", sizeof(float)},
   { "esf_s", "float", (void*) float_parser, offsetof(struct config, esf_s), "1.0", sizeof(float)},
   { "exclusion_radius", "float", (void*) float_parser, offsetof(struct config, exclusion_radius), "0.375", sizeof(float)},
   { "history", "integer", (void*) int_parser, offsetof(struct config, history), "0", sizeof(int)},
   { "hostname", "string", (void*) string_parser, offsetof(struct config, hostname), HOSTNAME, MAXVAL-1},
   { "listen_addr", "string", (void*) string_parser, offsetof(struct config, listen_addr), "127.0.0.1:10025", MAXVAL-1},
   { "log_subject", "integer", (void*) int_parser, offsetof(struct config, log_subject), "0", sizeof(int)},
   { "locale", "string", (void*) string_parser, offsetof(struct config, locale), "", MAXVAL-1},
   { "max_ham_spamicity", "float", (void*) float_parser, offsetof(struct config, max_ham_spamicity), "0.45", sizeof(float)},
   { "max_line_len", "integer", (void*) int_parser, offsetof(struct config, max_line_len), "2000", sizeof(int)},
   { "max_message_size_to_filter", "integer", (void*) int_parser, offsetof(struct config, max_message_size_to_filter), "256000", sizeof(int)},
   { "max_requests_per_child", "integer", (void*) int_parser, offsetof(struct config, max_requests_per_child), "10000", sizeof(int)},
   { "max_word_len", "integer", (void*) int_parser, offsetof(struct config, max_word_len), "25", sizeof(int)},
   { "message_from_a_zombie", "integer", (void*) int_parser, offsetof(struct config, message_from_a_zombie), "0", sizeof(int)},
   { "min_word_len", "integer", (void*) int_parser, offsetof(struct config, min_word_len), "1", sizeof(int)},
   { "mynetwork", "string", (void*) string_parser, offsetof(struct config, mynetwork), "", MAXVAL-1},
   { "mysqlhost", "string", (void*) string_parser, offsetof(struct config, mysqlhost), "", MAXVAL-1},
   { "mysqlport", "integer", (void*) int_parser, offsetof(struct config, mysqlport), "", sizeof(int)},
   { "mysqlsocket", "string", (void*) string_parser, offsetof(struct config, mysqlsocket), "/tmp/mysql.sock", MAXVAL-1},
   { "mysqluser", "string", (void*) string_parser, offsetof(struct config, mysqluser), "piler", MAXVAL-1},
   { "mysqlpwd", "string", (void*) string_parser, offsetof(struct config, mysqlpwd), "", MAXVAL-1},
   { "mysqldb", "string", (void*) string_parser, offsetof(struct config, mysqldb), "piler", MAXVAL-1},
   { "mysql_connect_timeout", "integer", (void*) int_parser, offsetof(struct config, mysql_connect_timeout), "2", sizeof(int)},
   { "number_of_worker_processes", "integer", (void*) int_parser, offsetof(struct config, number_of_worker_processes), "3", sizeof(int)},
   { "our_signo", "string", (void*) string_parser, offsetof(struct config, our_signo), "", MAXVAL-1},
   { "pemfile", "string", (void*) string_parser, offsetof(struct config, pemfile), "", MAXVAL-1},
   { "penalize_embed_images", "integer", (void*) int_parser, offsetof(struct config, penalize_embed_images), "0", sizeof(int)},
   { "penalize_images", "integer", (void*) int_parser, offsetof(struct config, penalize_images), "0", sizeof(int)},
   { "penalize_octet_stream", "integer", (void*) int_parser, offsetof(struct config, penalize_octet_stream), "0", sizeof(int)},
   { "pidfile", "string", (void*) string_parser, offsetof(struct config, pidfile), PIDFILE, MAXVAL-1},
   { "possible_spam_limit", "float", (void*) float_parser, offsetof(struct config, possible_spam_limit), "0.8", sizeof(float)},
   { "possible_spam_subject_prefix", "string", (void*) string_parser, offsetof(struct config, possible_spam_subject_prefix), "", MAXVAL-1},
   { "quarantinedir", "string", (void*) string_parser, offsetof(struct config, quarantinedir), QUARANTINE_DIR, MAXVAL-1},
   { "received_lines_to_skip", "integer", (void*) int_parser, offsetof(struct config, received_lines_to_skip), "0", sizeof(int)},
   { "rob_s", "float", (void*) float_parser, offsetof(struct config, rob_s), "1.0", sizeof(float)},
   { "rob_x", "float", (void*) float_parser, offsetof(struct config, rob_x), "0.52", sizeof(float)},
   { "skipped_received_ips", "string", (void*) string_parser, offsetof(struct config, skipped_received_ips), "", MAXVAL-1},
   { "smtp_addr", "string", (void*) string_parser, offsetof(struct config, smtp_addr), "127.0.0.1:10026", MAXVAL-1},
   { "spaminess_oblivion_limit", "float", (void*) float_parser, offsetof(struct config, spaminess_oblivion_limit), "1.01", sizeof(float)},
   { "spam_overall_limit", "float", (void*) float_parser, offsetof(struct config, spam_overall_limit), "0.92", sizeof(float)},
   { "spam_subject_prefix", "string", (void*) string_parser, offsetof(struct config, spam_subject_prefix), "", MAXVAL-1},
   { "update_tokens", "integer", (void*) int_parser, offsetof(struct config, update_tokens), "1", sizeof(int)},
   { "username", "string", (void*) string_parser, offsetof(struct config, username), "clapf", MAXVAL-1},
   { "verbosity", "integer", (void*) int_parser, offsetof(struct config, verbosity), "1", sizeof(int)},
   { "workdir", "string", (void*) string_parser, offsetof(struct config, workdir), WORK_DIR, MAXVAL-1},

   {NULL, NULL, NULL, 0, 0, 0}
};


/*
 * parse configfile
 */

int parse_config_file(char *configfile, struct config *target_cfg, struct _parse_rule *rules){
   char line[MAXVAL], *chpos;
   FILE *f;

   if(!configfile) return -1;

   f = fopen(configfile, "r");
   if(!f) return -1;

   while(fgets(&line[0], sizeof(line)-1, f)){
      if(line[0] == ';' || line[0] == '#') continue;

      chpos = strchr(line, '=');

      if(chpos){
         chop_newlines(chpos+1, strlen(chpos+1));
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


int load_default_config(struct config *cfg, struct _parse_rule *rules){
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

struct config read_config(char *configfile){
   struct config cfg;

   /* reset config structure and fill it with defaults */

   memset((char *)&cfg, 0, sizeof(struct config));

   load_default_config(&cfg, config_parse_rules);


   /* parse the config file */

   if(parse_config_file(configfile, &cfg, config_parse_rules) == -1) printf("error parsing the configfile: %s\n", configfile);

   return cfg;
}


/*
 * print a single configuration item as key=value
 */

void print_config_item(struct config *cfg, struct _parse_rule *rules, int i){
   int j;
   float f;
   char *p;

   p = (char*)cfg + rules[i].offset;

   if(strcmp(rules[i].type, "integer") == 0){
      memcpy((char*)&j, p, sizeof(int));
      printf("%s=%d\n", rules[i].name, j);
   }
   else if(strcmp(rules[i].type, "float") == 0){
      memcpy((char*)&f, p, sizeof(float));
      printf("%s=%.4f\n", rules[i].name, f);
   }
   else {
      chop_newlines(p, strlen(p));
      printf("%s=%s\n", rules[i].name, p);
   }

}


/*
 * print all known configuration items
 */

void print_config_all(struct config *cfg, char *key){
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

void print_config(char *configfile, struct config *cfg){
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
         chop_newlines(chpos+1, strlen(chpos+1));
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
