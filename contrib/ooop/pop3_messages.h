/*
 * pop3_messages.h, SJ
 */

#define AUTH_ERR_MISSING_USERNAME_OR_PASSWORD "-ERR Hianyzo felhasznalonev vagy jelszo\r\n"
#define AUTH_ERR_NO_COLON_IN_USERNAME "-ERR Nincs : a megadott felhasznalonevben\r\n"
#define AUTH_ERR_INTERNAL_SSL_PROBLEM "-ERR Belso SSL hiba\r\n"
#define AUTH_ERR_NO_SOCKET "-ERR Socket problema\r\n"
#define AUTH_ERR_INVALID_HOSTNAME "-ERR Ervenytelen POP3 kiszolgalo\r\n"
#define AUTH_ERR_CANNOT_CONNECT "-ERR Nem tudok kapcsolodni a valodi POP3 szerverhez\r\n"

#define INFO_HAM_FOUND_ON_WHITELIST "OK, feherlista"
#define INFO_SPAM_FOUND_ON_BLACKLIST "SPAM, feketelista"
#define INFO_SPAM_FOUND_ON_URL_BLACKLIST "SPAM, URL feketelista"
#define INFO_SPAM_BAYES "SPAM, bayes-i modul"

