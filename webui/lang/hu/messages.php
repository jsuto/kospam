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
$ADD_NEW_POLICY = "�j h�zirend";
$ADD_NEW_USER = "�j felhaszn�l�/$ALIAS hozz�ad�sa";
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
$LOGOUT = "Kijelentkez�s";

$MESSAGES = "levelek";
$MODIFY = "M�dos�t";

$NEXT = "K�vetkez�";
$NO_SENDER = "nincs felad�";
$NO_SUBJECT = "nincs t�rgy";
$NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE = "A spam �zenetek sz�ma a karant�nban";

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

$SELECT_ALL = "Mindegyik kijel�l�se";
$SET = "Be�ll�t";
$STATS = "Statisztika";
$STRING = "string";
$SUBJECT = "T�rgy";
$SUCCESSFULLY_REMOVED = "A t�rl�s siker�lt";

$TOTAL = "�sszes";
$TRAIN_AND_DELIVER = "Tan�t�s �s k�zbes�t�s";

$UPDATE_SELECTED_UIDS = "Kijel�lt azonos�t�k m�dos�t�sa";
$USERLIST = "Felhaszn�l�k list�ja";
$USERNAME = "Felhaszn�l�n�v";
$USERID = "Felhaszn�l� azonos�t�";
$USER_MANAGEMENT = "Felhaszn�l� menedzsel�s";

$WHITELIST = "Feh�rlista";
$WHITELIST_SETTINGS = "Feh�rlista be�ll�t�sa";

$YOU_ARE = "�n";


// error messages

$err_not_authenticated = "�n nem azonos�totta mag�t";
$err_connect_db = "Adatb�zis kapcsolat hiba";
$err_sql_error = "SQL hiba";
$err_NaN = "Nem sz�m";
$err_successfully_modified = "Sikeresen m�dos�tva";
$err_invalid_message_id = "Helytelen �zenet azonos�t�";

$err_existing_user = "A felhaszn�l� m�r l�tezik";
$err_added_user_successfully = "Felhaszn�l� hozz�ad�sa siker�lt";
$err_failed_to_remove_user = "Nem siker�lt a felhaszn�l� t�rl�se";
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

$err_you_are_logged_out = "�n sikeresen kijelentkezett";

$err_please_select_uids = "K�rem, v�lasszon felhaszn�l�i azonos�t�kat";
?>
