<?php

// titles

$TITLE['/index.php'] = 'clapf web UI - Kezdőlap';
$TITLE['/whitelist.php'] = 'Fehérlista';
$TITLE['/blacklist.php'] = 'Feketelista';
$TITLE['/stat.php'] = 'Statisztika';
$TITLE['/q.php'] = 'Karantén';
$TITLE['/users.php'] = 'Felhasználó menedzselés';
$TITLE['/policy.php'] = 'Házirend';
$TITLE['/logout.php'] = 'Kijelentkezés';

$GRAPH['ham_and_spam_messages'] = 'Ham & spam üzenetek száma';
$GRAPH['spam_ratio'] = 'Spam arány';

$ALIAS = "álnév";

$ADD = "Felvesz";
$ADD_NEW_EMAIL = "Új email cím";
$ADD_NEW_POLICY = "Új házirend";
$ADD_NEW_USER = "Új felhasználó/$ALIAS hozzáadása";
$ADMIN_USER = "Admin felhasználó";
$ALIASES = "Email álnevek";

$BACK = "Vissza";
$BLACKLIST = "Feketelista";
$BLACKLIST_SETTINGS = "Feketelista beállítások";
$BULK_EDIT_SELECTED_UIDS = "Kiválasztott azonosítók szerkesztése";
$BULK_USER_UPDATE = "Kiválasztott azonosítók szerkesztése";

$CANCEL = "Mégse";
$CGI_DAILY_REPORT = "Napi riport";
$CGI_MONTHLY_REPORT = "Havi riport";

$DATE = "Dátum";
$default_policy = "alapértelmezett";
$DELIVER = "Kézbesítés";
$DELIVERED = "Kézbesített";
$DELIVER_SELECTED_MESSAGES = "Kiválasztott üzenetek kézbesítése";
$DELIVER_AND_TRAIN_SELECTED_MESSAGES = "Kiválasztott üzenetek tanítása és kézbesítése";

$EDIT_OR_VIEW = "Szerkeszt/Megnéz";
$EMAIL_ADDRESS = "Email cím";
$ERROR = "Hiba";
$EXISTING_POLICY = "Létező házirendek";
$EXISTING_USERS = "Létező felhasználók";

$FAILED_TO_REMOVE = "Hiba az eltávolításkor";
$FIRST = "Első";
$FLOAT = "float";
$FROM = "Feladó";

$HOME = "Kezdőlap";

$INTEGER = "integer";

$LAST = "Utolsó";
$LOGIN = "Bejelentkezés";
$LOGOUT = "Kijelentkezés";

$MESSAGES = "levelek";
$MODIFY = "Módosít";

$NEXT = "Következő";
$NEW_EMAIL_ADDRESS = "Új email cím";
$NO_SENDER = "nincs feladó";
$NO_SUBJECT = "nincs tárgy";
$NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE = "A spam üzenetek száma a karanténban";

$PASSWORD = "Jelszó";
$PASSWORD_AGAIN = "Jelszó ismét";
$PREVIOUS = "Előző";
$PURGE_ALL_MESSAGES_FROM_QUARANTINE = "Összes üzenet törlése a karanténból";
$PURGE_SELECTED_MESSAGES = "Kiválasztott üzenetek eltávolítása";
$PURGED = "Eltávolítva";

$QUARANTINE = "Karantén";

$PAGE_LENGTH = "Lap méret";
$POLICY = "Házirend";
$POLICY_GROUP = "Házirend csoport";
$POLICY_NAME = "Házirend neve";

$REMOVE = "Törlés";
$REMOVE_POLICY = "Házirend törlése";
$REMOVE_SELECTED_UIDS = "Kijelölt azonosítók törlése";
$REMOVE_USER = "Felhasználó/$ALIAS törlése";

$SEARCH = "Keresés";
$SELECT_ALL = "Mindegyik kijelölése";
$SET = "Beállít";
$STATS = "Statisztika";
$STRING = "string";
$SUBJECT = "Tárgy";
$SUBMIT = "Mehet";
$SUCCESSFULLY_REMOVED = "A törlés sikerült";

$TOTAL = "Összes";
$TRAIN_AND_DELIVER = "Tanítás és kézbesítés";

$UPDATE_SELECTED_UIDS = "Kijelölt azonosítók módosítása";
$USERLIST = "Felhasználók listája";
$USERNAME = "Felhasználónév";
$USERID = "Felhasználó azonosító";
$USER_MANAGEMENT = "Felhasználó menedzselés";

$VIEW_FORMATTED_EMAIL = "Formázott levél megtekintése";
$VIEW_RAW_EMAIL = "Formázatlan levél megtekintése";

$WHITELIST = "Fehérlista";
$WHITELIST_SETTINGS = "Fehérlista beállítása";

$YOU_ARE = "Ön";


// error messages

$err_not_authenticated = "Ön nem azonosította magát, kérjük, <a href=\"$base_url/login.php\">jelentkezzen be</a>";
$err_connect_db = "Adatbázis kapcsolat hiba";
$err_sql_error = "SQL hiba";
$err_NaN = "Nem szám";
$err_successfully_modified = "Sikeresen módosítva";
$err_invalid_message_id = "Helytelen üzenet azonosító";

$err_existing_user = "A felhasználó már létezik";
$err_existing_email = "Az email cím már létezik";
$err_added_user_successfully = "Felhasználó hozzáadása sikerült";
$err_added_email_successfully = "Email cím hozzáadása sikerült";
$err_failed_to_remove_user = "Nem sikerült a felhasználó törlése";
$err_failed_to_remove_email = "Nem sikerült az email törlése";
$err_removed_user_successfully = "Felhasználó törlése sikerült";
$err_modified_user = "Felhasználó módosítva";
$err_failed_to_add_user = "Nem sikerült a felhasználó felvétele";

$err_you_are_not_admin = "Ön nem adminisztrátor szintű felhasználó";

$err_message_delivered = "Üzenet sikeresen továbbítva: ";
$err_message_failed_to_deliver = "Sikertelen üzenet továbbítás: ";
$err_message_failed_to_train = "Sikertelen tanítás";

$err_cannot_view_default_policy = "Az alapértelmezett házirend a clapf.conf állományban nézhető/módosítható";
$err_added_new_policy = "Új házirend felvéve";
$err_removed_policy = "Házirend törölve";
$err_modified_policy = "Házirend módosítva";
$err_policy_failed_to_modify = "Házirendet nem sikerült módosítani";
$err_policy_failed_to_add = "Házirendet nem sikerült felvenni";
$err_policy_failed_to_remove = "Házirendet nem sikerült törölni";

$err_this_feature_is_not_available = "Ez a funkció nem érhető el a használt adatbázissal";

$err_you_are_logged_out = "Ön sikeresen kijelentkezett. <a href=\"login.php\">$LOGIN</a>";

$err_please_select_uids = "Kérem, válasszon felhasználói azonosítókat";

$err_password_mismatch = "A jelszavak nem egyeznek meg";
$err_too_short_password = "Túl rövid jelszó";
$err_password_changed = "Jelszó megváltoztatva";
$err_failed_to_change_password = "Nem sikerült megváltoztatni a jelszót";

$err_empty_quarantine_directory_structure = "Üres karantén könyvtár struktúra";

?>
