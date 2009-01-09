/*
 * parser.h, 2009.01.09, SJ
 */

#ifndef _PARSER_H
 #define _PARSER_H

#include "cfg.h"
#include "config.h"
#include "defs.h"

void init_state(struct _state *state);
int attachment_by_type(struct _state *state, char *type);
int extract_boundary(char *p, char *boundary, int boundary_len);
int parse(char *buf, struct _state *state, struct session_data *sdata, struct __config *cfg);

#endif /* _PARSER_H */
