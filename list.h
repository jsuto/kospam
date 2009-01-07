/*
 * list.h, 2008.05.26, SJ
 */

#ifndef _LIST_H
 #define _LIST_H

#include "parser.h"

int append_url(struct _state *state, char *p);
struct url *new_url(char *s);
void free_url_list(struct url *u);

#endif /* _LIST_H */

