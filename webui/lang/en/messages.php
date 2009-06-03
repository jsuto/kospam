<?php

// titles

$TITLE['/index.php'] = 'clapf web UI - Home';
$TITLE['/whitelist.php'] = 'Whitelist';
$TITLE['/blacklist.php'] = 'Blacklist';
$TITLE['/stat.php'] = 'Statistics';
$TITLE['/q.php'] = 'Quarantine';
$TITLE['/users.php'] = 'User management';
$TITLE['/policy.php'] = 'Policy';
$TITLE['/logout.php'] = 'Logout';

$GRAPH['ham_and_spam_messages'] = 'Ham & spam messages';
$GRAPH['spam_ratio'] = 'Spam ratio';

$ALIAS = "alias";

$ADD = "Add";
$ADD_NEW_EMAIL = "Add new email address";
$ADD_NEW_POLICY = "Add new policy";
$ADD_NEW_USER = "Add new user/$ALIAS";
$ADMIN_USER = "Admin user";
$ALIASES = "Aliases";

$BACK = "Back";
$BLACKLIST = "Blacklist";
$BLACKLIST_SETTINGS = "Blacklist settings";
$BULK_EDIT_SELECTED_UIDS = "Bulk edit selected uids";
$BULK_USER_UPDATE = "Bulk update selected uids";

$CANCEL = "Cancel";
$CGI_DAILY_REPORT = "Daily report";
$CGI_MONTHLY_REPORT = "Monthly report";

$DATE = "Date";
$default_policy = "default_policy";
$DELIVER = "Deliver";
$DELIVERED = "Delivered";
$DELIVER_SELECTED_MESSAGES = "Deliver selected messages";
$DELIVER_AND_TRAIN_SELECTED_MESSAGES = "Deliver and train selected messages";

$EDIT_OR_VIEW = "Edit/view";
$EMAIL_ADDRESS = "Email address";
$ERROR = "Error";
$EXISTING_POLICY = "Existing policies";
$EXISTING_USERS = "Existing users";

$FAILED_TO_REMOVE = "Failed to remove";
$FIRST = "First";
$FLOAT = "float";
$FROM = "From";

$HOME = "Home";

$INTEGER = "integer";

$LAST = "Last";
$LOGIN = "Login";
$LOGOUT = "Logout";

$MESSAGES = "messages";
$MODIFY = "Modify";

$NEXT = "Next";
$NEW_EMAIL_ADDRESS = "New email address";
$NO_SENDER = "no sender";
$NO_SUBJECT = "no subject";
$NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE = "Number of spam messages in the quarantine";

$PASSWORD = "Password";
$PASSWORD_AGAIN = "Password again";
$PREVIOUS = "Previous";
$PURGE_ALL_MESSAGES_FROM_QUARANTINE = "Purge all messages from quarantine";
$PURGE_SELECTED_MESSAGES = "Purge selected messages";
$PURGED = "Purged";

$QUARANTINE = "Quarantine";

$PAGE_LENGTH = "Page length";
$POLICY = "Policy";
$POLICY_GROUP = "Policy group";
$POLICY_NAME = "Policy name";

$REMOVE = "Remove";
$REMOVE_POLICY = "Remove this policy";
$REMOVE_SELECTED_UIDS = "Remove selected uids";
$REMOVE_USER = "Remove this user/$ALIAS";

$SEARCH = "Search";
$SELECT_ALL = "Select all";
$SET = "Set";
$STATS = "Statistics";
$STRING = "string";
$SUBJECT = "Subject";
$SUBMIT = "Submit";
$SUCCESSFULLY_REMOVED = "Successfully removed";

$TOTAL = "Total";
$TRAIN_AND_DELIVER = "Train and deliver";

$UPDATE_SELECTED_UIDS = "Update selected uids";
$USERLIST = "User's list";
$USERNAME = "Username";
$USERID = "User id";
$USER_MANAGEMENT = "User management";

$VIEW_FORMATTED_EMAIL = "View formatted email";
$VIEW_RAW_EMAIL = "View raw email";

$WHITELIST = "Whitelist";
$WHITELIST_SETTINGS = "Whitelist settings";

$YOU_ARE = "You are";


// error messages

$err_not_authenticated = "You are not authenticated, please <a href=\"$base_url/login.php\">log in</a>";
$err_connect_db = "Database connection error";
$err_sql_error = "SQL error";
$err_NaN = "Not a number";
$err_successfully_modified = "Successfully modified";
$err_invalid_message_id = "Invalid message id";

$err_existing_user = "User already exists";
$err_existing_email = "Email already exists";
$err_added_user_successfully = "User added successfully";
$err_added_email_successfully = "Email added successfully";
$err_failed_to_remove_user = "User failed to remove";
$err_failed_to_remove_email = "Email failed to remove";
$err_removed_user_successfully = "User(s) removed successfully";
$err_modified_user = "Modified user";
$err_failed_to_add_user = "User failed to add";

$err_you_are_not_admin = "You are not an admin user";

$err_message_delivered = "Message delivered to";
$err_message_failed_to_deliver = "Message failed to deliver to";
$err_message_failed_to_train = "Message failed to train";

$err_cannot_view_default_policy = "The default policy can be view/edit in clapf.conf";
$err_added_new_policy = "Added new policy";
$err_removed_policy = "Removed policy";
$err_modified_policy = "Modified policy";
$err_policy_failed_to_modify = "Policy failed to modify";
$err_policy_failed_to_add = "Policy failed to add";
$err_policy_failed_to_remove = "Policy failed to remove";

$err_this_feature_is_not_available = "This feature is not available for this backend";

$err_you_are_logged_out = "You are logged out. <a href=\"login.php\">$LOGIN</a>";

$err_please_select_uids = "Please select some uids first";

$err_password_mismatch = "Password mismatch";
$err_too_short_password = "Too short password";
$err_password_changed = "Password changed";
$err_failed_to_change_password = "Failed to change password";

?>
