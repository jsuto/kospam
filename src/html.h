
struct html_tag {
   unsigned char length;
   char *entity;
};

#define NUM_OF_SKIP_TAGS 3

struct html_tag skip_html_tags[] = {
   { 4, "html" },
   { 5, "/html" },
   { 5, "/body" }
};

