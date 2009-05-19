<?php

function extract_mysql_dsn($dsn){
   $host = $u = $p = $db = "";

   $aa = substr($dsn, strlen("mysql://"), strlen($dsn));

   list($a1, $a2) = explode("@", $aa);

   list($u, $p) = explode(":", $a1);
   list($host, $db) = explode("/", $a2);

   return array ($host, $u, $p, $db);
}


function webui_connect(){
   global $dsn, $err_connect_db;

   list ($host, $u, $p, $db) = extract_mysql_dsn($dsn);

   $conn = mysql_connect($host, $u, $p) or nice_error($err_connect_db);
   mysql_select_db($db) or nice_error($err_connect_db);
   
   return $conn;
}


function webui_close($conn){
}


function print_email_addresses_for_user($conn, $username){
   global $user_table, $err_sql_error;

   $stmt = "SELECT email FROM $user_table WHERE username='$username' ORDER BY email";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   while(list($email) = mysql_fetch_row($r)){
      print "<tr><td>$email</td></tr>\n";
   }
   mysql_free_result($r);

}


function get_uid_by_name($username){
   global $user_table, $err_sql_error;
   $uid = -1;

   if($admin_user == 1) return 0;

   $stmt = "SELECT uid FROM $user_table WHERE username='$username'";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($uid) = mysql_fetch_row($r);
   mysql_free_result($r);

   return $uid;
}


function get_next_uid(){
   global $user_table, $err_sql_error;
   $uid = "";

   $stmt = "SELECT MAX(uid) FROM $user_table";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($uid) = mysql_fetch_row($r);
   mysql_free_result($r);

   if($uid > 0) $uid++;
   else $uid = 1;

   return $uid;
}


function get_whitelist_by_name($username){
   global $whitelist_table, $err_sql_error;
   $whitelist = "";

   $uid = get_uid_by_name($username);
   if($uid == "") return $whitelist;

   $stmt = "SELECT whitelist FROM $whitelist_table WHERE uid=$uid";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($whitelist) = mysql_fetch_row($r);
   mysql_free_result($r);

   return $whitelist;
}


function set_whitelist($whitelist, $username){
   global $whitelist_table, $err_sql_error;

   $uid = get_uid_by_name($username);

   $whitelist = mysql_real_escape_string($whitelist);

   $uuid = mysql_result(mysql_query("SELECT COUNT(*) AS `uid` FROM $whitelist_table WHERE `uid` = '$uid'"), 0, 'uid');
   if(is_numeric($uuid) && $uuid > 0)
      $stmt = "UPDATE $whitelist_table SET whitelist='$whitelist' WHERE uid=$uid";
   else
      $stmt = "INSERT INTO $whitelist_table (uid, whitelist) VALUES($uid, '$whitelist')";

   mysql_query($stmt) or nice_error($err_sql_error);
}


function get_blacklist_by_name($username){
   global $blacklist_table, $err_sql_error;
   $blacklist = "";

   $uid = get_uid_by_name($username);
   if($uid == "") return $blacklist;

   $stmt = "SELECT blacklist FROM $blacklist_table WHERE uid=$uid";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($blacklist) = mysql_fetch_row($r);
   mysql_free_result($r);

   return $blacklist;
}


function set_blacklist($blacklist, $username){
   global $blacklist_table, $err_sql_error;

   $uid = get_uid_by_name($username);

   $blacklist = mysql_real_escape_string($blacklist);

   $uuid = mysql_result(mysql_query("SELECT COUNT(*) AS `uid` FROM $blacklist_table WHERE `uid` = '$uid'"), 0, 'uid');
   if(is_numeric($uuid) && $uuid > 0)
      $stmt = "UPDATE $blacklist_table SET blacklist='$blacklist' WHERE uid=$uid";
   else
      $stmt = "INSERT INTO $blacklist_table (uid, blacklist) VALUES($uid, '$blacklist')";

   mysql_query($stmt) or nice_error($err_sql_error);
}


/*** users ***/


function check_user_auth($username, $password){
   global $user_table, $err_sql_error;
   $p = "";
   $ok = 0;

   webui_connect() or nice_error($err_connect_db);

   $username = mysql_real_escape_string($username);

   $stmt = "SELECT password FROM $user_table WHERE username='$username'";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($p) = mysql_fetch_row($r);
   mysql_free_result($r);

   if($p){
      $pass = crypt($password, $p);
      if($pass == $p){
         $ok = 1;
         $_SESSION['username'] = $username;
      }
   }

   webui_close($conn);

   return $ok;
}


function is_admin_user($username){
   global $user_table, $err_sql_error;
   $isadmin = 0;

   if($username == "") return $isadmin;

   $conn = webui_connect() or nice_error($err_connect_db);

   $username = mysql_real_escape_string($username);

   $stmt = "SELECT isadmin FROM $user_table WHERE username='$username'";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($isadmin) = mysql_fetch_row($r);
   mysql_free_result($r);

   webui_close($conn);

   return $isadmin;
}


function get_users_email_address($username){
   global $user_table, $email_table, $err_sql_error;
   $to = "";

   $stmt = "SELECT $email_table.email FROM $email_table, $user_table WHERE $email_table.uid=$user_table.uid AND $user_table.username='$username' LIMIT 1";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($to) = mysql_fetch_row($r);
   mysql_free_result($r);

   return $to;
}


function get_user_entry($uid, $email = ""){
   global $user_table, $email_table, $whitelist_table, $blacklist_table, $err_sql_error;

   $x = array();

   if(!is_numeric($uid) || $uid < 0) return $x;

   $email = mysql_real_escape_string($email);
   if($email) $EMAIL = " AND $email_table.email='$email'";

   $stmt = "SELECT $email_table.email, $user_table.username, $user_table.uid, $user_table.policy_group, $user_table.isadmin FROM $user_table, $email_table WHERE $user_table.uid=$uid $EMAIL ORDER by uid, email";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   $x = mysql_fetch_row($r);
   mysql_free_result($r);

   $stmt = "SELECT whitelist FROM $whitelist_table WHERE uid=$uid";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($whitelist) = mysql_fetch_row($r);
   mysql_free_result($r);

   $stmt = "SELECT blacklist FROM $blacklist_table WHERE uid=$uid";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($blacklist) = mysql_fetch_row($r);
   mysql_free_result($r);

   array_push($x, $whitelist, $blacklist);

   return $x;
}


function print_user($x, $ro_uid = 0){
   global $EMAIL_ADDRESS, $USERNAME, $PASSWORD, $USERID, $POLICY_GROUP, $ADMIN_USER, $WHITELIST, $BLACKLIST, $default_policy;

   $len = 30;

   print "<tr><td>$EMAIL_ADDRESS:</td><td><input type=\"text\" name=\"email\" value=\"$x[0]\"></td></tr>\n";
   print "<tr><td>$USERNAME:</td><td><input type=\"text\" name=\"username\" value=\"$x[1]\"></td></tr>\n";
   print "<tr><td>$PASSWORD:</td><td><input type=\"password\" name=\"password\" value=\"$x[6]\"></td></tr>\n";

   if($ro_uid == 1)
      print "<tr><td>$USERID:</td><td>$x[2]</td></tr>\n";
   else
      print "<tr><td>$USERID:</td><td><input type=\"text\" name=\"uid\" value=\"$x[2]\"></td></tr>\n";

   print "<tr><td>$POLICY_GROUP:</td><td>\n";

   print "<select name=\"policy_group\">\n";
   print "<option value=\"0\">$default_policy</option>\n";
   show_existing_policy_groups($x[3]);
   print "</select>\n";

   print "</td></tr>\n";

   print "<tr><td>$ADMIN_USER:</td><td>\n";

   print "<select name=\"isadmin\">\n";
   show_yes_or_no($x[4]);
   print "</select>\n";

   print "</td></tr>\n";

   print "<tr valign=\"top\"><td>$WHITELIST:</td><td><textarea name=\"whitelist\" cols=\"$len\" rows=\"5\">$x[5]</textarea></td></tr>\n";
   print "<tr valign=\"top\"><td>$BLACKLIST:</td><td><textarea name=\"blacklist\" cols=\"$len\" rows=\"5\">$x[6]</textarea></td></tr>\n";

}


function show_existing_users($what, $page, $page_len){
   global $user_table, $email_table, $err_sql_error, $EDIT_OR_VIEW;
   $where_cond = "WHERE $user_table.uid=$email_table.uid ";
   $n_users = 0;
   $from = $page * $page_len;

   if($what){
      $what = mysql_real_escape_string($what);
      $where_cond .= "AND ($user_table.username LIKE '%$what%' OR $email_table.email LIKE '%$what%') ";
   }

   $stmt = "SELECT COUNT(*) FROM $user_table,$email_table $where_cond";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list ($n_users) = mysql_fetch_row($r);
   mysql_free_result($r);

   $stmt = "SELECT $user_table.uid, $user_table.username, $user_table.policy_group, $email_table.email FROM $user_table, $email_table  $where_cond ORDER by $user_table.uid LIMIT $from, $page_len";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   while(list($uid, $username, $policy_group, $email) = mysql_fetch_row($r)){
      $policy_group = get_policy_group_name_by_id($policy_group);

      print "<tr align=\"left\"><td><input type=\"checkbox\" name=\"aa_$uid\"></td><td>$uid</td><td>$username</td><td>$email</td><td>$policy_group</td><td><a href=\"users.php?uid=$uid&email=$email&edit=1\">$EDIT_OR_VIEW</a></td></tr>\n";
   }
   mysql_free_result($r);

   return $n_users;
}


function delete_existing_user_entry($uid, $email){
   global $user_table, $email_table, $whitelist_table, $blacklist_table, $misc_table, $err_sql_error, $BACK, $err_failed_to_remove_user;

   $uid = mysql_real_escape_string($uid);
   $email = mysql_real_escape_string($email);

   /* determine if this is the last user entry */

   $stmt = "SELECT COUNT(*) FROM $email_table WHERE uid=$uid";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($n) = mysql_fetch_row($r);
   mysql_free_result($r);

   $stmt = "DELETE FROM $email_table WHERE uid=$uid AND email='$email'";
   if(!mysql_query($stmt)) nice_error($err_failed_to_remove_email . ". <a href=\"users.php\">$BACK.</a>");

   if($n == 1 && $uid > 0){
      $stmt = "DELETE FROM $user_table WHERE uid=$uid";
      mysql_query($stmt);
      $stmt = "DELETE FROM $whitelist_table WHERE uid=$uid";
      mysql_query($stmt);
      $stmt = "DELETE FROM $blacklist_table WHERE uid=$uid";
      mysql_query($stmt);
      $stmt = "DELETE FROM $misc_table WHERE uid=$uid";
      mysql_query($stmt);
   }
}


function bulk_delete_user($uidlist){
   global $user_table, $whitelist_table, $blacklist_table, $err_sql_error;

   reset($_POST);
   while(list($k, $v) = each($_POST)) $$k = mysql_real_escape_string($v);

   $uidlist = mysql_real_escape_string($uidlist);

   $stmt = "DELETE FROM $email_table WHERE uid IN ($uidlist)";
   mysql_query($stmt) or nice_error($err_sql_error);

   $stmt = "DELETE FROM $user_table WHERE uid IN ($uidlist)";
   mysql_query($stmt) or nice_error($err_sql_error);

   $stmt = "DELETE FROM $whitelist_table WHERE uid IN ($uidlist)";
   mysql_query($stmt) or nice_error($err_sql_error);

   $stmt = "DELETE FROM $blacklist_table  WHERE uid IN ($uidlist)";
   mysql_query($stmt) or nice_error($err_sql_error);
}


function add_user_entry($uid){
   global $user_table, $email_table, $whitelist_table, $blacklist_table, $misc_table, $err_sql_error, $err_existing_user, $BACK;

   while(list($k, $v) = each($_POST)) $$k = mysql_real_escape_string($v);

   $uid = mysql_real_escape_string($uid);

   $stmt = "INSERT INTO $user_table (uid, username, password, policy_group, isadmin) VALUES($uid, '$username', ENCRYPT('$password'), $policy_group, $isadmin)";
   if(!mysql_query($stmt)) nice_error($err_existing_user . ". <a href=\"users.php\">$BACK.</a>");

   $stmt = "INSERT INTO $email_table (uid, email) VALUES($uid, '$email')";
   if(!mysql_query($stmt)) nice_error($err_existing_email . ". <a href=\"users.php\">$BACK.</a>");

   $stmt = "INSERT INTO $whitelist_table (uid, whitelist) VALUES($uid, '$whitelist')";
   mysql_query($stmt);

   $stmt = "INSERT INTO $blacklist_table (uid, blacklist) VALUES($uid, '$blacklist')";
   mysql_query($stmt);

   $stmt = "INSERT INTO $misc_table (uid, nham, nspam) VALUES($uid, 0, 0)";
   mysql_query($stmt);
}


function update_user($uid){
   global $user_table, $email_table, $whitelist_table, $blacklist_table, $err_sql_error;

   $PWD = "";

   while(list($k, $v) = each($_POST)) $$k = mysql_real_escape_string($v);

   if($password) $PWD = ", password=ENCRYPT('$password')";

   $stmt = "UPDATE $user_table SET username='$username', policy_group=$policy_group, isadmin=$isadmin $PWD WHERE uid=$uid";
   mysql_query($stmt) or nice_error($err_sql_error);

   $stmt = "UPDATE $email_table SET email='$email' WHERE uid=$uid AND email='$email_orig'";
   mysql_query($stmt) or nice_error($err_sql_error);

   $uuid = mysql_result(mysql_query("SELECT COUNT(*) AS `uid` FROM $whitelist_table WHERE `uid` = '$uid'"), 0, 'uid');
   if(is_numeric($uuid) && $uuid > 0)
      $stmt = "UPDATE $whitelist_table SET whitelist='$whitelist' WHERE uid=$uid";
   else
      $stmt = "INSERT INTO $whitelist_table (uid, whitelist) VALUES($uid, '$whitelist')";

   mysql_query($stmt) or nice_error($err_sql_error);

   $uuid = mysql_result(mysql_query("SELECT COUNT(*) AS `uid` FROM $blacklist_table WHERE `uid` = '$uid'"), 0, 'uid');
   if(is_numeric($uuid) && $uuid > 0)
      $stmt = "UPDATE $blacklist_table SET blacklist='$blacklist' WHERE uid=$uid";
   else
      $stmt = "INSERT INTO $blacklist_table (uid, blacklist) VALUES($uid, '$blacklist')";

   mysql_query($stmt) or nice_error($err_sql_error);

}


function change_password(){
   global $user_table, $username, $err_user_failed_to_modify;

   $c_pwd = crypt($_POST['password']);

   $stmt = "UPDATE $user_table SET password='$c_pwd' WHERE username='$username'";

   $conn = webui_connect() or nice_error($err_connect_db);

   mysql_query($stmt) or nice_error($err_sql_error);

   webui_close($conn);

   return 1;
}


function bulk_update_user($uidlist){
   global $user_table, $whitelist_table, $blacklist_table, $err_sql_error;

   reset($_POST);
   while(list($k, $v) = each($_POST)) $$k = mysql_real_escape_string($v);

   $uidlist = mysql_real_escape_string($uidlist);

   $stmt = "UPDATE $user_table SET policy_group=$policy_group WHERE uid IN ($uidlist)";
   mysql_query($stmt) or nice_error($err_sql_error);

   $stmt = "UPDATE $whitelist_table SET whitelist='$whitelist' WHERE uid IN ($uidlist)";
   mysql_query($stmt) or nice_error($err_sql_error);

   $stmt = "UPDATE $blacklist_table SET blacklist='$blacklist' WHERE uid IN ($uidlist)";
   mysql_query($stmt) or nice_error($err_sql_error);

}


/*** policy groups ***/


function show_existing_policy_groups($id = 0){
   global $policy_group_table, $err_sql_error, $REMOVE;

   $stmt = "SELECT policy_group, name FROM $policy_group_table ORDER by policy_group";

   $r = mysql_query($stmt) or nice_error($err_sql_error);
   while(list($policy_group, $name) = mysql_fetch_row($r)){
      if($policy_group == $id)
         print "<option value=\"$policy_group\" selected>$name</option>\n";
      else
         print "<option value=\"$policy_group\">$name</option>\n";
   }
   mysql_free_result($r);
}


function get_policy_group_name_by_id($id){
   global $policy_group_table, $err_sql_error, $default_policy;

   if($id == 0) return $default_policy;

   $stmt = "SELECT name FROM $policy_group_table WHERE policy_group = $id";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($name) = mysql_fetch_row($r);
   mysql_free_result($r);

   return $name;
}


function get_new_policy_group_id(){
   global $policy_group_table, $err_sql_error;

   $stmt = "SELECT MAX(policy_group) FROM $policy_group_table";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($n) = mysql_fetch_row($r);
   mysql_free_result($r);

   if($n == "") $n = 1;
   else $n++;

   return $n;
}


function show_policy($policy_group){
   global $policy_group_table, $err_sql_error, $POLICY, $MODIFY, $CANCEL;

   $x = array();

   $stmt = "SELECT name, deliver_infected_email, silently_discard_infected_email, use_antispam, spam_subject_prefix, enable_auto_white_list, max_message_size_to_filter, rbl_domain, surbl_domain, spam_overall_limit, spaminess_oblivion_limit, replace_junk_characters, invalid_junk_limit, invalid_junk_line, penalize_images, penalize_embed_images, penalize_octet_stream, training_mode, initial_1000_learning, store_metadata FROM $policy_group_table WHERE policy_group=$policy_group";

   $r = mysql_query($stmt) or nice_error($err_sql_error);
   $x = mysql_fetch_row($r);
   mysql_free_result($r);

   print "<table>\n";

   print_policy($x);

   print "<tr><td><input type=\"submit\" value=\"$MODIFY\"> <input type=\"reset\" value=\"$CANCEL\"></td><td></td></tr>\n";
   print "</table>\n";

}


function add_policy(){
   global $policy_group_table, $err_sql_error;

   while(list($k, $v) = each($_POST)) $$k = mysql_real_escape_string($v);

   $policy_group = get_new_policy_group_id();

   $stmt = "INSERT INTO $policy_group_table (policy_group, name, deliver_infected_email, silently_discard_infected_email, use_antispam, spam_subject_prefix, enable_auto_white_list, max_message_size_to_filter, rbl_domain, surbl_domain, spam_overall_limit, spaminess_oblivion_limit, replace_junk_characters, invalid_junk_limit, invalid_junk_line, penalize_images, penalize_embed_images, penalize_octet_stream, training_mode, initial_1000_learning, store_metadata) VALUES($policy_group, '$name', $deliver_infected_email, $silently_discard_infected_email, $use_antispam, '$spam_subject_prefix', $enable_auto_white_list, $max_message_size_to_filter, '$rbl_domain', '$surbl_domain', $spam_overall_limit, $spaminess_oblivion_limit, $replace_junk_characters, $invalid_junk_limit, $invalid_junk_line, $penalize_images, $penalize_embed_images, $penalize_octet_stream, $training_mode, $initial_1000_learning, $store_metadata)";

   mysql_query($stmt) or nice_error($err_sql_error);
}


function update_policy($policy_group){
   global $policy_group_table, $err_sql_error, $POLICY, $MODIFY, $CANCEL;

   while(list($k, $v) = each($_POST)) $$k = mysql_real_escape_string($v);

   $stmt = "UPDATE $policy_group_table SET name='$name', deliver_infected_email=$deliver_infected_email, silently_discard_infected_email=$silently_discard_infected_email, use_antispam=$use_antispam, spam_subject_prefix='$spam_subject_prefix', enable_auto_white_list=$enable_auto_white_list, max_message_size_to_filter=$max_message_size_to_filter, rbl_domain='$rbl_domain', surbl_domain='$surbl_domain', spam_overall_limit=$spam_overall_limit, spaminess_oblivion_limit=$spaminess_oblivion_limit, replace_junk_characters=$replace_junk_characters, invalid_junk_limit=$invalid_junk_limit, invalid_junk_line=$invalid_junk_line, penalize_images=$penalize_images, penalize_embed_images=$penalize_embed_images, penalize_octet_stream=$penalize_octet_stream, training_mode=$training_mode, initial_1000_learning=$initial_1000_learning, store_metadata=$store_metadata WHERE policy_group=$policy_group";

   mysql_query($stmt) or nice_error($err_sql_error);
}


function remove_policy($policy_group){
   global $policy_group_table, $err_sql_error;

   $stmt = "DELETE FROM $policy_group_table WHERE policy_group=$policy_group";
   mysql_query($stmt) or nice_error($err_sql_error);
}


?>
