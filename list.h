/*
 * list.h, 2009.01.22, SJ
 */

#ifndef _LIST_H
 #define _LIST_H

#include "parser.h"

int append_list(struct url **urls, char *p);
struct url *new_list(char *s);
void free_list(struct url *u);

#endif /* _LIST_H */

