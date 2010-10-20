/*
 * users.h, SJ
 */

#ifndef _USERS_H
 #define _USERS_H

int getUserdataFromEmail(struct session_data *sdata, char *email, struct __config *cfg);
int isKnownEmail(struct session_data *sdata, char *email, struct __config *cfg);

#endif /* _USERS_H */

