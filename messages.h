/*
 * messages.h, 2008.07.14, SJ
 */

#ifndef _MESSAGES_H
 #define _MESSAGES_H

// cgi

#define ERR_CGI_INVALID_METHOD "Invalid method"

#define CGI_SPAM_QUARANTINE "Spam Quarantine"
#define ERR_CGI_DELIVER_AND_REMOVE "Deliver and remove"
#define ERR_CGI_REMOVE "Remove"
#define ERR_CGI_NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE "Total number of spam messages"

#define ERR_CGI_DELIVER_AND_TRAIN_AS_HAM "Deliver, train as HAM, then remove"
#define ERR_CGI_TRAIN_AS_HAM "Train as HAM, then remove"

#define ERR_CGI_INVALID_REQUEST "Invalid CGI request"
#define ERR_CGI_NO_MEMORY "No memory to allocate"
#define ERR_CGI_POST_READ "Cannot read POST input"
#define ERR_CGI_NO_MESSAGE "No message found"

#define ERR_CGI_CANNOT_OPEN "Cannot open"
#define ERR_CGI_DELIVERED_AND_REMOVED "Message delivered and removed"
#define ERR_CGI_REMOVED "Message removed"
#define ERR_CGI_FAILED_TO_REMOVE "Failed to remove"
#define ERR_CGI_DELIVERY_FAILED "Delivery failed"
#define ERR_CGI_INVALID_ID "Invalid message id"

#define ERR_CGI_PURGE_SELECTED "Purge selected messages"
#define ERR_CGI_CANCEL "Cancel"
#define ERR_CGI_SELECT_ALL "Select all"
#define ERR_CGI_PURGED_MESSAGES "Purged messages"
#define ERR_CGI_BACK "Back"

#define ERR_CGI_YOU_NOT_ADMIN "You are not admin"

#define ERR_CGI_I_TAUGHT "Thanks! I taught the token database"
#define ERR_CGI_DELIVERED_REMOVED_AND_TRAINED_AS_HAM "Message delivered, trained as HAM and removed"
#define ERR_CGI_REMOVED_AND_TRAINED_AS_HAM "Message trained as HAM and removed"

#define ERR_CGI_EXISTING_USER "Already existing user"
#define ERR_CGI_NEW_USER_SUCCESSFULLY_ADDED "Added new user successfully"
#define ERR_CGI_CANNOT_REMOVE "Cannot remove"
#define ERR_CGI_USER_SUCCESSFULLY_REMOVED "Removed successfully"
#define ERR_CGI_CANNOT_MODIFY "Cannot modify user"
#define ERR_CGI_NEW_USER_SUCCESSFULLY_MODIFIED "User successfully modified"

#define CGI_USER_MANAGEMENT "User management"

#define CGI_DATE "Date"
#define CGI_FROM "From"
#define CGI_SUBJECT "Subject"
#define CGI_USER "User"
#define CGI_EMAIL "Email address"
#define CGI_USERID "User id"
#define CGI_MESSAGE "Message"
#define CGI_HAM_OR_SPAM "Ham/Spam"
#define CGI_USER_LIST "Users' Spam Quarantine List"
#define CGI_ADD_NEW_USER "Add new user"

// mysql

#define ERR_MYSQL_CONNECT "Cannot connect to mysql server"
#define ERR_SQL_DATA "No valid data from sql table"
#define ERR_CGI_MYSQL_NO_USER "User not found in user table"

// sqlite3

#define ERR_SQLITE3_OPEN "Cannot open sqlite3 database"

#define ERR_MYDB_OPEN "Cannot init mydb database"

// spam quarantine

#define ERR_CANNOT_OPEN "Cannot open"
#define ERR_CANNOT_CHDIR "Cannot chdir"
#define ERR_CGI_NOT_AUTHENTICATED "You are not authenticated"

// train

#define ERR_OPEN_MESSGAE_FILE "Cannot open message file"
#define ERR_INVALID_MODE "Invalid training mode"
#define ERR_TRAIN_DONE "Training has done"
#define ERR_TRAINING "training"
#define ERR_NO_FROM "No From environment variable"
#define ERR_TRAIN_AS_HAMSPAM "cannot train both as spam and as ham"
#define CGI_TRAIN_LOG "Training log"


// user preferences

#define CGI_USER_PREF "User preferences"
#define ERR_CGI_USERPREF_UPDATED "User preferences has been updated"
#define ERR_CGI_USERPREF_UPDATE_FAILED "Failed to update user preferences"

#define ERR_INVALID_UID "Invalid uid"
#define ERR_EXISTING_UID "Existing uid"

// statistics

#define CGI_PERSONAL_STAT "Statistics"
#define CGI_DAILY_REPORT "Daily report"
#define CGI_MONTHLY_REPORT "Monthly report"

// reason 

#define MSG_STRANGE_LANGUAGE "message has strange language characters"
#define MSG_TOO_MUCH_SPAM_IN_TOP15 "message has too many spammy tokens in the top15"
#define MSG_BLACKHOLED "message sender is in our blackhole"
#define MSG_CAUGHT_BY_SURBL "message reached surbl threshold"
#define MSG_TEXT_AND_BASE64 "messages is text but base64 encoded"
#define MSG_EMBED_IMAGE "message has embedded image"
#define MSG_ABSOLUTELY_SPAM "message is absolutely spam"


#endif /* _MESSGAES_H */

