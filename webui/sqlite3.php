<?

function webui_connect(){
   global $sqlite3_db, $err_connect_db;

   try{
      $conn = new PDO("sqlite:$sqlite3_db");
   } catch(PDOException $exception){
      //die($exception->getMessage());
      nice_error($err_connect_db);
   }

   return $conn;
}


function webui_close($conn){
}


function print_email_addresses_for_user($conn, $username){
   global $user_table, $err_sql_error;

   $stmt = "SELECT email FROM $user_table WHERE username='$username' ORDER BY email";

   $result = $conn->query($stmt);
   $r = $result->fetchAll();

   while(list($k, $v) = each($r)){
      print "<tr><td>$v[0]</td></tr>\n";
   }

}


function get_uid_by_name($username){
   global $conn, $user_table, $err_sql_error;
   $uid = -1;

   if($admin_user == 1) return 0;

   $stmt = "SELECT uid FROM $user_table WHERE username='$username'";

   $result = $conn->query($stmt);
   $r = $result->fetch();
   $uid = $r[0];

   return $uid;
}


function get_next_uid(){
   global $conn, $user_table, $err_sql_error;
   $uid = "";

   $stmt = "SELECT MAX(uid) FROM $user_table";

   $result = $conn->query($stmt);
   $r = $result->fetch();
   $uid = $r[0];

   if($uid > 0) $uid++;
   else $uid = 1;

   return $uid;
}


function get_whitelist_by_name($username){
   global $conn, $whitelist_table, $err_sql_error;
   $whitelist = "";

   $uid = get_uid_by_name($username);

   $stmt = "SELECT whitelist FROM $whitelist_table WHERE uid=$uid";

   $result = $conn->query($stmt);
   $r = $result->fetch();
   $whitelist = $r[0];

   return $whitelist;
}


function set_whitelist($whitelist, $username){
   global $conn, $whitelist_table, $err_sql_error;

   $uid = get_uid_by_name($username);

   $stmt = "UPDATE $whitelist_table SET whitelist=:whitelist WHERE uid=:uid";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':whitelist', $whitelist, PDO::PARAM_STR);
   $r->execute();
}


/*** users ***/


function get_users_email_address($username){
   global $conn, $user_table, $err_sql_error;
   $to = "";

   $stmt = "SELECT email FROM $user_table WHERE username='$username' LIMIT 1";

   $result = $conn->query($stmt);
   $r = $result->fetch();
   $to = $r[0];

   return $to;
}


function print_user($x, $ro_uid = 0){
   global $conn, $EMAIL_ADDRESS, $USERNAME, $USERID, $POLICY_GROUP, $WHITELIST, $default_policy;

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
   //show_existing_policy_groups($x[3]);
   print "</select>\n";

   print "<tr valign=\"top\"><td>$WHITELIST:</td><td><textarea name=\"whitelist\" cols=\"$len\" rows=\"5\">$x[4]</textarea></td></tr>\n";

   print "</td></tr>\n";

}


function get_user_entry($uid, $email = ""){
   global $conn, $user_table, $whitelist_table, $err_sql_error;

   $x = array();

   if(!is_numeric($uid) || $uid < 1) return $x;

   if($email) $EMAIL = " AND email=:email";

   $stmt = "SELECT email, username, uid, policy_group FROM $user_table WHERE uid=:uid $EMAIL ORDER by uid, email";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   if($email) $r->bindParam(':email', $email, PDO::PARAM_STR);
   $r->execute();
   $R = $r->fetch();

   array_push($x, $R['email']);
   array_push($x, $R['username']);
   array_push($x, $R['uid']);
   array_push($x, $R['policy_group']);

   $stmt = "SELECT whitelist FROM $whitelist_table WHERE uid=:uid";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->execute();
   $R = $r->fetch();

   array_push($x, $R['whitelist']);

   return $x;
}


function show_existing_users(){
   global $conn, $user_table, $err_sql_error, $EDIT_OR_VIEW;

   $stmt = "SELECT uid, username, email, policy_group FROM $user_table ORDER by uid, email";

   $result = $conn->query($stmt);
   $r = $result->fetchAll();

   while(list($k, $v) = each($r)){
      $uid = $v['uid'];
      $username = $v['username'];
      $email = $v['email'];
      $policy_group = $v['policy_group'];

      //$policy_group = get_policy_group_name_by_id($policy_group);

      print "<tr align=\"left\"><td>$uid</td><td>$username</td><td>$email</td><td>$policy_group</td><td><a href=\"users.php?uid=$uid&email=$email&edit=1\">$EDIT_OR_VIEW</a></td></tr>\n";
   }

}


function delete_existing_user_entry($uid, $email){
   global $conn, $user_table, $whitelist_table, $misc_table, $err_sql_error, $BACK, $err_failed_to_remove_user;

   /* determine if this is the last user entry */

   $stmt = "SELECT COUNT(*) FROM $user_table WHERE uid=:uid";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->execute();
   $R = $r->fetch();

   $n = $R[0];

   $stmt = "DELETE FROM $user_table WHERE uid=:uid AND email=:email";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':email', $email, PDO::PARAM_STR);
   $r->execute() or nice_error($err_failed_to_remove_user . ". <a href=\"users.php\">$BACK.</a>");

   if($n == 1 && $uid > 0){
      $stmt = "DELETE FROM $whitelist_table WHERE uid=:uid";
      $r = $conn->prepare($stmt);
      $r->bindParam(':uid', $uid, PDO::PARAM_INT);
      $r->execute();

      $stmt = "DELETE FROM $misc_table WHERE uid=:uid";
      $r = $conn->prepare($stmt);
      $r->bindParam(':uid', $uid, PDO::PARAM_INT);
      $r->execute();
   }
}


function add_user_entry($uid){
   global $conn, $user_table, $whitelist_table, $misc_table, $err_sql_error, $err_existing_user, $BACK;

   while(list($k, $v) = each($_POST)) $$k = $v;

   $stmt = "INSERT INTO $user_table (uid, username, email, policy_group) VALUES(:uid, :username, :email, :policy_group)";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':username', $username, PDO::PARAM_STR);
   $r->bindParam(':email', $email, PDO::PARAM_STR);
   $r->bindParam(':policy_group', $policy_group, PDO::PARAM_INT);
   $r->execute() or nice_error($err_existing_user . ". <a href=\"users.php\">$BACK.</a>");

   $stmt = "INSERT INTO $whitelist_table (uid, whitelist) VALUES(:uid, :whitelist)";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':whitelist', $whitelist, PDO::PARAM_STR);
   $r->execute();
         
   $stmt = "INSERT INTO $misc_table (uid, nham, nspam) VALUES(:uid, 0, 0)";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->execute();

}


function update_user($uid){
   global $conn, $user_table, $whitelist_table, $err_sql_error;

   while(list($k, $v) = each($_POST)) $$k = $v;

   $stmt = "UPDATE $user_table SET username=:username, email=:email, policy_group=:policy_group WHERE uid=:uid AND email=:email_orig";

   $r = $conn->prepare($stmt);
   $r->bindParam(':username', $username, PDO::PARAM_STR);
   $r->bindParam(':email', $email, PDO::PARAM_STR);
   $r->bindParam(':policy_group', $policy_group, PDO::PARAM_INT);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':email_orig', $email_orig, PDO::PARAM_STR);
   $r->execute() or nice_error($err_sql_error);


   $stmt = "UPDATE $whitelist_table SET whitelist=:whitelist WHERE uid=:uid";
   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':whitelist', $whitelist, PDO::PARAM_STR);
   $r->execute() or nice_error($err_sql_error);
}



/*** policy groups ***/


function show_existing_policy_groups($id = 0){
   global $policy_group_table, $err_sql_error, $REMOVE;
}


