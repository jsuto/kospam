/*
 * list.h, 2008.05.26, SJ
 */

#ifndef _LIST_H
 #define _LIST_H

#include "parser.h"

void insert_token(struct _state *state, char *p);
struct _token *new_token(char *s);
void free_and_print_list(struct _token *t, int print);

int append_url(struct _state *state, char *p);
struct url *new_url(char *s);
void free_url_list(struct url *u);

#endif /* _LIST_H */

