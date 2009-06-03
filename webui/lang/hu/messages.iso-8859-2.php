<?php

// titles

$TITLE['/index.php'] = 'clapf web UI - Kezd�lap';
$TITLE['/whitelist.php'] = 'Feh�rlista';
$TITLE['/blacklist.php'] = 'Feketelista';
$TITLE['/stat.php'] = 'Statisztika';
$TITLE['/q.php'] = 'Karant�n';
$TITLE['/users.php'] = 'Felhaszn�l� menedzsel�s';
$TITLE['/policy.php'] = 'H�zirend';
$TITLE['/logout.php'] = 'Kijelentkez�s';

$GRAPH['ham_and_spam_messages'] = 'Ham & spam �zenetek sz�ma';
$GRAPH['spam_ratio'] = 'Spam ar�ny';

$ALIAS = "�ln�v";

$ADD = "Felvesz";
$ADD_NEW_EMAIL = "�j email c�m";
$ADD_NEW_POLICY = "�j h�zirend";
$ADD_NEW_USER = "�j felhaszn�l�/$ALIAS hozz�ad�sa";
$ADMIN_USER = "Admin felhaszn�l�";
$ALIASES = "Email �lnevek";

$BACK = "Vissza";
$BLACKLIST = "Feketelista";
$BLACKLIST_SETTINGS = "Feketelista be�ll�t�sok";
$BULK_EDIT_SELECTED_UIDS = "Kiv�lasztott azonos�t�k szerkeszt�se";
$BULK_USER_UPDATE = "Kiv�lasztott azonos�t�k szerkeszt�se";

$CANCEL = "M�gse";
$CGI_DAILY_REPORT = "Napi riport";
$CGI_MONTHLY_REPORT = "Havi riport";

$DATE = "D�tum";
$default_policy = "alap�rtelmezett";
$DELIVER = "K�zbes�t�s";
$DELIVERED = "K�zbes�tett";
$DELIVER_SELECTED_MESSAGES = "Kiv�lasztott �zenetek k�zbes�t�se";
$DELIVER_AND_TRAIN_SELECTED_MESSAGES = "Kiv�lasztott �zenetek tan�t�sa �s k�zbes�t�se";

$EDIT_OR_VIEW = "Szerkeszt/Megn�z";
$EMAIL_ADDRESS = "Email c�m";
$ERROR = "Hiba";
$EXISTING_POLICY = "L�tez� h�zirendek";
$EXISTING_USERS = "L�tez� felhaszn�l�k";

$FAILED_TO_REMOVE = "Hiba az elt�vol�t�skor";
$FIRST = "Els�";
$FLOAT = "float";
$FROM = "Felad�";

$HOME = "Kezd�lap";

$INTEGER = "integer";

$LAST = "Utols�";
$LOGIN = "Bejelentkez�s";
$LOGOUT = "Kijelentkez�s";

$MESSAGES = "levelek";
$MODIFY = "M�dos�t";

$NEXT = "K�vetkez�";
$NEW_EMAIL_ADDRESS = "�j email c�m";
$NO_SENDER = "nincs felad�";
$NO_SUBJECT = "nincs t�rgy";
$NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE = "A spam �zenetek sz�ma a karant�nban";

$PASSWORD = "Jelsz�";
$PASSWORD_AGAIN = "Jelsz� ism�t";
$PREVIOUS = "El�z�";
$PURGE_ALL_MESSAGES_FROM_QUARANTINE = "�sszes �zenet t�rl�se a karant�nb�l";
$PURGE_SELECTED_MESSAGES = "Kiv�lasztott �zenetek elt�vol�t�sa";
$PURGED = "Elt�vol�tva";

$QUARANTINE = "Karant�n";

$PAGE_LENGTH = "Lap m�ret";
$POLICY = "H�zirend";
$POLICY_GROUP = "H�zirend csoport";
$POLICY_NAME = "H�zirend neve";

$REMOVE = "T�rl�s";
$REMOVE_POLICY = "H�zirend t�rl�se";
$REMOVE_SELECTED_UIDS = "Kijel�lt azonos�t�k t�rl�se";
$REMOVE_USER = "Felhaszn�l�/$ALIAS t�rl�se";

$SEARCH = "Keres�s";
$SELECT_ALL = "Mindegyik kijel�l�se";
$SET = "Be�ll�t";
$STATS = "Statisztika";
$STRING = "string";
$SUBJECT = "T�rgy";
$SUBMIT = "Mehet";
$SUCCESSFULLY_REMOVED = "A t�rl�s siker�lt";

$TOTAL = "�sszes";
$TRAIN_AND_DELIVER = "Tan�t�s �s k�zbes�t�s";

$UPDATE_SELECTED_UIDS = "Kijel�lt azonos�t�k m�dos�t�sa";
$USERLIST = "Felhaszn�l�k list�ja";
$USERNAME = "Felhaszn�l�n�v";
$USERID = "Felhaszn�l� azonos�t�";
$USER_MANAGEMENT = "Felhaszn�l� menedzsel�s";

$VIEW_FORMATTED_EMAIL = "Form�zott lev�l megtekint�se";
$VIEW_RAW_EMAIL = "Form�zatlan lev�l megtekint�se";

$WHITELIST = "Feh�rlista";
$WHITELIST_SETTINGS = "Feh�rlista be�ll�t�sa";

$YOU_ARE = "�n";


// error messages

$err_not_authenticated = "�n nem azonos�totta mag�t, k�rj�k, <a href=\"$base_url/login.php\">jelentkezzen be</a>";
$err_connect_db = "Adatb�zis kapcsolat hiba";
$err_sql_error = "SQL hiba";
$err_NaN = "Nem sz�m";
$err_successfully_modified = "Sikeresen m�dos�tva";
$err_invalid_message_id = "Helytelen �zenet azonos�t�";

$err_existing_user = "A felhaszn�l� m�r l�tezik";
$err_existing_email = "Az email c�m m�r l�tezik";
$err_added_user_successfully = "Felhaszn�l� hozz�ad�sa siker�lt";
$err_added_email_successfully = "Email c�m hozz�ad�sa siker�lt";
$err_failed_to_remove_user = "Nem siker�lt a felhaszn�l� t�rl�se";
$err_failed_to_remove_email = "Nem siker�lt az email t�rl�se";
$err_removed_user_successfully = "Felhaszn�l� t�rl�se siker�lt";
$err_modified_user = "Felhaszn�l� m�dos�tva";
$err_failed_to_add_user = "Nem siker�lt a felhaszn�l� felv�tele";

$err_you_are_not_admin = "�n nem adminisztr�tor szint� felhaszn�l�";

$err_message_delivered = "�zenet sikeresen tov�bb�tva: ";
$err_message_failed_to_deliver = "Sikertelen �zenet tov�bb�t�s: ";
$err_message_failed_to_train = "Sikertelen tan�t�s";

$err_cannot_view_default_policy = "Az alap�rtelmezett h�zirend a clapf.conf �llom�nyban n�zhet�/m�dos�that�";
$err_added_new_policy = "�j h�zirend felv�ve";
$err_removed_policy = "H�zirend t�r�lve";
$err_modified_policy = "H�zirend m�dos�tva";
$err_policy_failed_to_modify = "H�zirendet nem siker�lt m�dos�tani";
$err_policy_failed_to_add = "H�zirendet nem siker�lt felvenni";
$err_policy_failed_to_remove = "H�zirendet nem siker�lt t�r�lni";

$err_this_feature_is_not_available = "Ez a funkci� nem �rhet� el a haszn�lt adatb�zissal";

$err_you_are_logged_out = "�n sikeresen kijelentkezett. <a href=\"login.php\">$LOGIN</a>";

$err_please_select_uids = "K�rem, v�lasszon felhaszn�l�i azonos�t�kat";

$err_password_mismatch = "A jelszavak nem egyeznek meg";
$err_too_short_password = "T�l r�vid jelsz�";
$err_password_changed = "Jelsz� megv�ltoztatva";
$err_failed_to_change_password = "Nem siker�lt megv�ltoztatni a jelsz�t";

$err_empty_quarantine_directory_structure = "�res karant�n k�nyvt�r strukt�ra";
$err_non_existent_queue_directory = "A megadott karant�n k�nyvt�r nem l�tezik";

?>
