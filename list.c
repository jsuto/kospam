/*
 * list.c, 2009.01.22, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parser.h"
#include "list.h"
#include "config.h"


/*
 * append something to list if we have to
 */

int append_list(struct url **urls, char *p){
   struct url *q, *t, *u=NULL;

   q = *urls;

   while(q){
      if(strcmp(q->url_str, p) == 0)
         return 0;

      u = q;
      q = q->r;
   }

   t = new_list(p);
   if(t){
      if(*urls == NULL)
         *urls = t;
      else if(u)
         u->r = t;

      return 1;
   }

   return -1;
}



/*
 * create a new url structure
 */

struct url *new_list(char *s){
   struct url *h=NULL;

   if((h = malloc(sizeof(struct url))) == NULL)
      return NULL;

   strncpy(h->url_str, s, URL_LEN-1);
   h->r = NULL;

   return h;
}


/*
 * free url list
 */

void free_list(struct url *u){
   struct url *p, *q;

   p = u;

   while(p != NULL){
      q = p->r;

      if(p)
         free(p);

      p = q;
   }
}



