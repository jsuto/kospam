/*
 * prefs.h, 2008.06.04, SJ
 */

#ifndef _PREFS_H
 #define _PREFS_H

#include "cfg.h"


int get_user_preferences(char *login, char *prefs_db, struct __config *cfg, unsigned long *activation_date, char **whitelist);

#endif /* _PREFS_H */

