/*
 * users.h, SJ
 */

#ifndef _USERS_H
 #define _USERS_H

int get_user_data_from_email(struct session_data *sdata, struct __data *data, char *email, struct __config *cfg);
void get_wbl_data(struct session_data *sdata, struct __data *data, struct __config *cfg);

#endif /* _USERS_H */

