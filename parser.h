/*
 * parser.h, 2009.01.09, SJ
 */

#ifndef _PARSER_H
 #define _PARSER_H

#include "cfg.h"
#include "config.h"
#include "defs.h"

#define MSG_UNDEF -1
#define MSG_BODY 0
#define MSG_RECEIVED 1
#define MSG_FROM 2
#define MSG_TO 3
#define MSG_SUBJECT 4
#define MSG_CONTENT_TYPE 5
#define MSG_CONTENT_TRANSFER_ENCODING 6
#define MSG_CONTENT_DISPOSITION 7


void init_state(struct _state *state);
int attachment_by_type(struct _state *state, char *type);
int extract_boundary(char *p, char *boundary, int boundary_len);
int parse(char *buf, struct _state *state, struct session_data *sdata, struct __config *cfg);

#endif /* _PARSER_H */
