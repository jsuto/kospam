<?

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


function get_users_email_address($username){
   global $user_table, $err_sql_error;
   $to = "";

   $stmt = "SELECT email FROM $user_table WHERE username='$username' LIMIT 1";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($to) = mysql_fetch_row($r);
   mysql_free_result($r);

   return $to;
}


function show_existing_users(){
   global $user_table, $err_sql_error, $REMOVE;

   $stmt = "SELECT uid, username, email FROM $user_table ORDER by uid, email";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   while(list($uid, $username, $email) = mysql_fetch_row($r)){
      print "<tr align=\"left\"><td>$uid</td><td>$username</td><td>$email</td><td><a href=\"users.php?remove=1&uid=$uid&email=$email\">$REMOVE</a></td></tr>\n";
   }
   mysql_free_result($r);
}


function delete_existing_user_entry($uid, $email){
   global $user_table, $err_sql_error, $BACK, $err_failed_to_remove_user;

   $uid = mysql_real_escape_string($uid);
   $email = mysql_real_escape_string($email);

   $stmt = "DELETE FROM $user_table WHERE uid=$uid AND email='$email'";
   if(!mysql_query($stmt)) nice_error($err_failed_to_remove_user . ". <a href=\"users.php\">$BACK.</a>");

   /* TODO: remove his entry from the whitelist table only if this was his last entry */
}


function add_user_entry($u, $user, $mail){
   global $user_table, $whitelist_table, $misc_table, $err_sql_error, $err_existing_user, $BACK;

   $uid = mysql_real_escape_string($u);
   $username = mysql_real_escape_string($user);
   $email = mysql_real_escape_string($mail);

   $stmt = "INSERT INTO $user_table (uid, username, email) VALUES($uid, '$username', '$email')";
   if(!mysql_query($stmt)) nice_error($err_existing_user . ". <a href=\"users.php\">$BACK.</a>");

   $stmt = "INSERT INTO $whitelist_table (uid) VALUES($uid)";
   mysql_query($stmt);
	 
   $stmt = "INSERT INTO $misc_table (uid, nham, nspam) VALUES($uid, 0, 0)";
   mysql_query($stmt);
}

?>
