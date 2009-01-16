<?php

function webui_connect(){
   global $host, $u, $p, $db, $err_connect_db;

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
   $stmt = "UPDATE $whitelist_table SET whitelist='$whitelist' WHERE uid=$uid";
   mysql_query($stmt) or nice_error($err_sql_error);
}


/*** users ***/


function get_users_email_address($username){
   global $user_table, $err_sql_error;
   $to = "";

   $stmt = "SELECT email FROM $user_table WHERE username='$username' LIMIT 1";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($to) = mysql_fetch_row($r);
   mysql_free_result($r);

   return $to;
}


function print_user($x, $ro_uid = 0){
   global $EMAIL_ADDRESS, $USERNAME, $USERID, $POLICY_GROUP, $WHITELIST, $default_policy;

   $len = 30;

   print "<tr><td>$EMAIL_ADDRESS:</td><td><input type=\"text\" name=\"email\" value=\"$x[0]\"></td></tr>\n";
   print "<tr><td>$USERNAME:</td><td><input type=\"text\" name=\"username\" value=\"$x[1]\"></td></tr>\n";

   if($ro_uid == 1)
      print "<tr><td>$USERID:</td><td>$x[2]</td></tr>\n";
   else
      print "<tr><td>$USERID:</td><td><input type=\"text\" name=\"uid\" value=\"$x[2]\"></td></tr>\n";

   print "<tr><td>$POLICY_GROUP:</td><td>\n";

   print "<select name=\"policy_group\">\n";
   print "<option value=\"0\">$default_policy</option>\n";
   show_existing_policy_groups($x[3]);
   print "</select>\n";

   print "<tr valign=\"top\"><td>$WHITELIST:</td><td><textarea name=\"whitelist\" cols=\"$len\" rows=\"5\">$x[4]</textarea></td></tr>\n";

   print "</td></tr>\n";

}


function get_user_entry($uid, $email = ""){
   global $user_table, $whitelist_table, $err_sql_error;

   $x = array();

   if(!is_numeric($uid) || $uid < 1) return $x;

   $email = mysql_real_escape_string($email);
   if($email) $EMAIL = " AND email='$email'";

   $stmt = "SELECT email, username, uid, policy_group FROM $user_table WHERE uid=$uid $EMAIL ORDER by uid, email";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   $x = mysql_fetch_row($r);
   mysql_free_result($r);

   $stmt = "SELECT whitelist FROM $whitelist_table WHERE uid=$uid";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($whitelist) = mysql_fetch_row($r);
   mysql_free_result($r);

   array_push($x, $whitelist);

   return $x;
}


function show_existing_users(){
   global $user_table, $err_sql_error, $EDIT_OR_VIEW;

   $stmt = "SELECT uid, username, email, policy_group FROM $user_table ORDER by uid, email";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   while(list($uid, $username, $email, $policy_group) = mysql_fetch_row($r)){
      $policy_group = get_policy_group_name_by_id($policy_group);

      print "<tr align=\"left\"><td>$uid</td><td>$username</td><td>$email</td><td>$policy_group</td><td><a href=\"users.php?uid=$uid&email=$email&edit=1\">$EDIT_OR_VIEW</a></td></tr>\n";
   }
   mysql_free_result($r);
}


function delete_existing_user_entry($uid, $email){
   global $user_table, $whitelist_table, $misc_table, $err_sql_error, $BACK, $err_failed_to_remove_user;

   $uid = mysql_real_escape_string($uid);
   $email = mysql_real_escape_string($email);

   /* determine if this is the last user entry */

   $stmt = "SELECT COUNT(*) FROM $user_table WHERE uid=$uid";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($n) = mysql_fetch_row($r);
   mysql_free_result($r);

   $stmt = "DELETE FROM $user_table WHERE uid=$uid AND email='$email'";
   if(!mysql_query($stmt)) nice_error($err_failed_to_remove_user . ". <a href=\"users.php\">$BACK.</a>");

   if($n == 1 && $uid > 0){
      $stmt = "DELETE FROM $whitelist_table WHERE uid=$uid";
      mysql_query($stmt);
      $stmt = "DELETE FROM $misc_table WHERE uid=$uid";
      mysql_query($stmt);
   }
}


function add_user_entry($uid){
   global $user_table, $whitelist_table, $misc_table, $err_sql_error, $err_existing_user, $BACK;

   while(list($k, $v) = each($_POST)) $$k = mysql_real_escape_string($v);

   $uid = mysql_real_escape_string($uid);

   $stmt = "INSERT INTO $user_table (uid, username, email, policy_group) VALUES($uid, '$username', '$email', $policy_group)";
   if(!mysql_query($stmt)) nice_error($err_existing_user . ". <a href=\"users.php\">$BACK.</a>");

   $stmt = "INSERT INTO $whitelist_table (uid, whitelist) VALUES($uid, '$whitelist')";
   mysql_query($stmt);
	 
   $stmt = "INSERT INTO $misc_table (uid, nham, nspam) VALUES($uid, 0, 0)";
   mysql_query($stmt);
}


function update_user($uid){
   global $user_table, $whitelist_table, $err_sql_error;

   while(list($k, $v) = each($_POST)) $$k = mysql_real_escape_string($v);

   $stmt = "UPDATE $user_table SET username='$username', email='$email', policy_group=$policy_group WHERE uid=$uid AND email='$email_orig'";
   mysql_query($stmt) or nice_error($err_sql_error);

   $stmt = "UPDATE $whitelist_table SET whitelist='$whitelist' WHERE uid=$uid";
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


function print_policy($x){
   global $POLICY, $POLICY_NAME, $INTEGER, $FLOAT, $STRING;

   print "<tr><td>$POLICY_NAME</td><td><b><input type=\"text\" name=\"name\" value=\"$x[0]\" size=\"30\"></b></td><td>&nbsp;</td></tr>\n";

   print "<tr><td>deliver_infected_email</td><td><input name=\"deliver_infected_email\" value=\"$x[1]\" size=\"3\"></td><td>0|1</td></tr>\n";
   print "<tr><td>silently_discard_infected_email</td><td><input name=\"silently_discard_infected_email\" value=\"$x[2]\" size=\"3\"></td><td>0|1</td></tr>\n";
   print "<tr><td>use_antispam</td><td><input name=\"use_antispam\" value=\"$x[3]\" size=\"3\"></td><td>0|1</td></tr>\n";
   print "<tr><td>spam_subject_prefix</td><td><input name=\"spam_subject_prefix\" value=\"$x[4]\" size=\"30\"></td><td>[$STRING]</td></tr>\n";
   print "<tr><td>enable_auto_white_list</td><td><input name=\"enable_auto_white_list\" value=\"$x[5]\" size=\"1\"></td><td>0|1</td></tr>\n";
   print "<tr><td>max_message_size_to_filter</td><td><input name=\"max_message_size_to_filter\" value=\"$x[6]\" size=\"6\"></td><td>$INTEGER</td></tr>\n";
   print "<tr><td>rbl_domain</td><td><input name=\"rbl_domain\" value=\"$x[7]\" size=\"30\"></td><td>$STRING</td></tr>\n";
   print "<tr><td>surbl_domain</td><td><input name=\"surbl_domain\" value=\"$x[8]\" size=\"30\"></td><td>$STRING</td></tr>\n";
   print "<tr><td>spam_overall_limit</td><td><input name=\"spam_overall_limit\" value=\"$x[9]\" size=\"3\"></td><td>$FLOAT</td></tr>\n";
   print "<tr><td>spaminess_oblivion_limit</td><td><input name=\"spaminess_oblivion_limit\" value=\"" . sprintf("%.2f", $x[10]) . "\" size=\"3\"></td><td>$FLOAT</td></tr>\n";
   print "<tr><td>replace_junk_characters</td><td><input name=\"replace_junk_characters\" value=\"$x[11]\" size=\"3\"><td>0|1</td></td></tr>\n";
   print "<tr><td>invalid_junk_limit</td><td><input name=\"invalid_junk_limit\" value=\"$x[12]\" size=\"3\"></td><td>$INTEGER</td></tr>\n";
   print "<tr><td>invalid_junk_line</td><td><input name=\"invalid_junk_line\" value=\"$x[13]\" size=\"3\"></td><td>$INTEGER</td></tr>\n";
   print "<tr><td>penalize_images</td><td><input name=\"penalize_images\" value=\"$x[14]\" size=\"3\"></td><td>0|1</td></tr>\n";
   print "<tr><td>penalize_embed_images</td><td><input name=\"penalize_embed_images\" value=\"$x[15]\" size=\"3\"></td><td>0|1</td></tr>\n";
   print "<tr><td>penalize_octet_stream</td><td><input name=\"penalize_octet_stream\" value=\"$x[16]\" size=\"3\"></td><td>0|1</td></tr>\n";
   print "<tr><td>training_mode</td><td><input name=\"training_mode\" value=\"$x[17]\" size=\"3\"></td><td>0|1</td></tr>\n";
   print "<tr><td>initial_1000_learning</td><td><input name=\"initial_1000_learning\" value=\"$x[18]\" size=\"3\"></td><td>0|1</td></tr>\n";

}

function show_policy($policy_group){
   global $policy_group_table, $err_sql_error, $POLICY, $MODIFY, $CANCEL;

   $x = array();

   $stmt = "SELECT name, deliver_infected_email, silently_discard_infected_email, use_antispam, spam_subject_prefix, enable_auto_white_list, max_message_size_to_filter, rbl_domain, surbl_domain, spam_overall_limit, spaminess_oblivion_limit, replace_junk_characters, invalid_junk_limit, invalid_junk_line, penalize_images, penalize_embed_images, penalize_octet_stream, training_mode, initial_1000_learning FROM $policy_group_table WHERE policy_group=$policy_group";

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

   $stmt = "INSERT INTO $policy_group_table (policy_group, name, deliver_infected_email, silently_discard_infected_email, use_antispam, spam_subject_prefix, enable_auto_white_list, max_message_size_to_filter, rbl_domain, surbl_domain, spam_overall_limit, spaminess_oblivion_limit, replace_junk_characters, invalid_junk_limit, invalid_junk_line, penalize_images, penalize_embed_images, penalize_octet_stream, training_mode, initial_1000_learning) VALUES($policy_group, '$name', $deliver_infected_email, $silently_discard_infected_email, $use_antispam, '$spam_subject_prefix', $enable_auto_white_list, $max_message_size_to_filter, '$rbl_domain', '$surbl_domain', $spam_overall_limit, $spaminess_oblivion_limit, $replace_junk_characters, $invalid_junk_limit, $invalid_junk_line, $penalize_images, $penalize_embed_images, $penalize_octet_stream, $training_mode, $initial_1000_learning)";

   mysql_query($stmt) or nice_error($err_sql_error);
}


function update_policy($policy_group){
   global $policy_group_table, $err_sql_error, $POLICY, $MODIFY, $CANCEL;

   while(list($k, $v) = each($_POST)) $$k = mysql_real_escape_string($v);

   $stmt = "UPDATE $policy_group_table SET name='$name', deliver_infected_email=$deliver_infected_email, silently_discard_infected_email=$silently_discard_infected_email, use_antispam=$use_antispam, spam_subject_prefix='$spam_subject_prefix', enable_auto_white_list=$enable_auto_white_list, max_message_size_to_filter=$max_message_size_to_filter, rbl_domain='$rbl_domain', surbl_domain='$surbl_domain', spam_overall_limit=$spam_overall_limit, spaminess_oblivion_limit=$spaminess_oblivion_limit, replace_junk_characters=$replace_junk_characters, invalid_junk_limit=$invalid_junk_limit, invalid_junk_line=$invalid_junk_line, penalize_images=$penalize_images, penalize_embed_images=$penalize_embed_images, penalize_octet_stream=$penalize_octet_stream, training_mode=$initial_1000_learning WHERE policy_group=$policy_group";

   mysql_query($stmt) or nice_error($err_sql_error);
}


function remove_policy($policy_group){
   global $policy_group_table, $err_sql_error;

   $stmt = "DELETE FROM $policy_group_table WHERE policy_group=$policy_group";
   mysql_query($stmt) or nice_error($err_sql_error);
}


?>
