/*
 * tokenizer.c, SJ
 */

#include <kospam.h>


int generate_tokens_from_string(struct parser_state *state, const char *s, char *label, struct config *cfg){
   int i=0, n=0;
   char tmpbuf[SMALLBUFSIZE], chain[SMALLBUFSIZE];

   while (s) {
      int result;
      char v[SMALLBUFSIZE];
      s = split((char *)s, ' ', v, sizeof(v)-1, &result);

      size_t len = strlen(v);

      // chop trailing dot, comma, etc
      char c = v[len-1];
      if (c == '!' || c == '\'' || c == '(' || c == ')' || c == ',' || c == '.' || c == ':' || c == '?') {
         v[len-1] = '\0';
         len--;
      }

      bool url = false;
      int skiplen = 0;

      if (strncasecmp(v, "http://", 7) == 0) {
         url = true;
         skiplen = 7;
      } else if (strncasecmp(v, "https://", 8) == 0) {
         url =  true;
         skiplen = 8;
      }

      if (url) {
         char tmp[SMALLBUFSIZE];
         extract_url_token(&v[skiplen], &tmp[0], sizeof(tmp));
         addnode(state->token_hash, tmp, DEFAULT_SPAMICITY, 0);
         state->n_token++;
      }

      // skip single letter or too long words, but keep attachment names
      if (len < (size_t)cfg->min_word_len || (len > (size_t)cfg->max_word_len && strcmp(label, "SUBJ*") && strncmp(&v[skiplen], "att*", 4)) ) continue;

      if(i > 0){
         snprintf(chain, sizeof(chain)-1, "%s+%s%s", tmpbuf, label, &v[skiplen]);
         addnode(state->token_hash, chain, DEFAULT_SPAMICITY, 0);
         n++;
      }

      snprintf(tmpbuf, sizeof(tmpbuf)-1, "%s%s", label, &v[skiplen]);
      if(len > 2){
         addnode(state->token_hash, tmpbuf, DEFAULT_SPAMICITY, 0);
         n++;
      }

      i++;
   }

   return n;
}
