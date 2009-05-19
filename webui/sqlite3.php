<?php

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


function get_blacklist_by_name($username){
   global $conn, $blacklist_table, $err_sql_error;
   $blacklist = "";

   $uid = get_uid_by_name($username);

   $stmt = "SELECT blacklist FROM $blacklist_table WHERE uid=$uid";

   $result = $conn->query($stmt);
   $r = $result->fetch();
   $whitelist = $r[0];

   return $blacklist;
}


function set_blacklist($blacklist, $username){
   global $conn, $blacklist_table, $err_sql_error;

   $uid = get_uid_by_name($username);

   $stmt = "UPDATE $blacklist_table SET blacklist=:blacklist WHERE uid=:uid";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':blacklist', $blacklist, PDO::PARAM_STR);
   $r->execute();
}



/*** users ***/


function check_user_auth($username, $password){
   global $user_table, $err_sql_error;
   $p = "";
   $ok = 0;

   $conn = webui_connect() or nice_error($err_connect_db);

   $stmt = "SELECT password FROM $user_table WHERE username='$username'";

   $r = $conn->prepare($stmt);
   $result = $conn->query($stmt);
   $r = $result->fetch();

   $p = $r[0];

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

   $conn = webui_connect() or nice_error($err_connect_db);

   $stmt = "SELECT isadmin FROM $user_table WHERE username='$username'";
   $result = $conn->query($stmt);
   $r = $result->fetch();

   $isadmin = $r[0];

   webui_close($conn);

   return $isadmin;
}


function get_users_email_address($username){
   global $conn, $user_table, $email_table, $err_sql_error;
   $to = "";

   $stmt = "SELECT $email_table.email FROM $email_table, $user_table WHERE $email_table.uid=$user_table.uid AND $user_table.username='$username' LIMIT 1";

   $result = $conn->query($stmt);
   $r = $result->fetch();
   $to = $r[0];

   return $to;
}


function get_user_entry($uid, $email = ""){
   global $conn, $user_table, $email_table, $whitelist_table, $blacklist_table, $err_sql_error;

   $x = array();

   if(!is_numeric($uid) || $uid < 0) return $x;

   if($email) $EMAIL = " AND $email_table.email=:email";

   $stmt = "SELECT $email_table.email, $user_table.username, $user_table.uid, $user_table.policy_group, $user_table.isadmin FROM $user_table, $email_table WHERE $user_table.uid=:uid $EMAIL";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   if($email) $r->bindParam(':email', $email, PDO::PARAM_STR);
   $r->execute();
   $R = $r->fetch();

   array_push($x, $R['email']);
   array_push($x, $R['username']);
   array_push($x, $R['uid']);
   array_push($x, $R['policy_group']);
   array_push($x, $R['isadmin']);

   $stmt = "SELECT whitelist FROM $whitelist_table WHERE uid=:uid";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->execute();
   $R = $r->fetch();

   array_push($x, $R['whitelist']);

   $stmt = "SELECT blacklist FROM $blacklist_table WHERE uid=:uid";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->execute();
   $R = $r->fetch();

   array_push($x, $R['blacklist']);

   return $x;
}


function print_user($x, $ro_uid = 0){
   global $conn, $EMAIL_ADDRESS, $USERNAME, $PASSWORD, $PASSWORD_AGAIN, $USERID, $POLICY_GROUP, $ADMIN_USER, $WHITELIST, $BLACKLIST, $default_policy;

   $len = 30;

   print "<tr><td>$EMAIL_ADDRESS:</td><td><input type=\"text\" name=\"email\" value=\"$x[0]\"></td></tr>\n";
   print "<tr><td>$USERNAME:</td><td><input type=\"text\" name=\"username\" value=\"$x[1]\"></td></tr>\n";
   print "<tr><td>$PASSWORD:</td><td><input type=\"password\" name=\"password\" value=\"\"></td></tr>\n";
   print "<tr><td>$PASSWORD_AGAIN:</td><td><input type=\"password\" name=\"password2\" value=\"\"></td></tr>\n";

   if($ro_uid == 1)
      print "<tr><td>$USERID:</td><td>$x[2]</td></tr>\n";
   else
      print "<tr><td>$USERID:</td><td><input type=\"text\" name=\"uid\" value=\"$x[2]\"></td></tr>\n";

   print "<tr><td>$POLICY_GROUP:</td><td>\n";

   print "<select name=\"policy_group\">\n";
   print "<option value=\"0\">$default_policy</option>\n";
   //show_existing_policy_groups($x[3]);
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
   global $conn, $user_table, $email_table, $err_sql_error, $EDIT_OR_VIEW, $NEW_EMAIL_ADDRESS;
   $where_cond = " WHERE $user_table.uid=$email_table.uid ";
   $n_users = 0;
   $from = $page * $page_len;

   if($what){
      $where_cond .= "AND ($user_table.username LIKE '%$what%' OR $email_table.email LIKE '%$what%') ";
   }

   $stmt = "SELECT COUNT(*) AS aaa FROM $user_table,$email_table $where_cond";
   $result = $conn->query($stmt);
   $r = $result->fetchAll();

   list($k, $v) = each($r);
   $n_users = $v['aaa']; 


   $stmt = "SELECT $user_table.uid, $user_table.username, $user_table.policy_group, $email_table.email FROM $user_table, $email_table  $where_cond ORDER by $user_table.uid LIMIT $from, $page_len";

   $result = $conn->query($stmt);
   $r = $result->fetchAll();

   while(list($k, $v) = each($r)){
      $uid = $v['uid'];
      $username = $v['username'];
      $email = $v['email'];
      $policy_group = $v['policy_group'];

      $policy_group = get_policy_group_name_by_id($policy_group);

      print "<tr align=\"left\"><td><input type=\"checkbox\" name=\"aa_$uid\"></td><td>$uid</td><td>$username</td><td>$email</td><td>$policy_group</td><td><a href=\"users.php?uid=$uid&email=$email&edit=1\">$EDIT_OR_VIEW</a></td><td><a href=\"emails.php?uid=$uid&username=$username&add=1\">$NEW_EMAIL_ADDRESS</a></td></tr>\n";
   }

   return $n_users;
}


function delete_existing_user_entry($uid, $email){
   global $conn, $user_table, $email_table, $whitelist_table, $blacklist_table, $misc_table, $err_sql_error, $BACK, $err_failed_to_remove_user;

   /* determine if this is the last user entry */

   $stmt = "SELECT COUNT(*) FROM $email_table WHERE uid=:uid";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->execute();
   $R = $r->fetch();

   $n = $R[0];

   $stmt = "DELETE FROM $email_table WHERE uid=:uid AND email=:email";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':email', $email, PDO::PARAM_STR);
   $r->execute() or nice_error($err_failed_to_remove_email . ". <a href=\"users.php\">$BACK.</a>");

   if($n == 1 && $uid > 0){
      $stmt = "DELETE FROM $user_table WHERE uid=:uid";
      $r = $conn->prepare($stmt);
      $r->bindParam(':uid', $uid, PDO::PARAM_INT);
      $r->execute();

      $stmt = "DELETE FROM $whitelist_table WHERE uid=:uid";
      $r = $conn->prepare($stmt);
      $r->bindParam(':uid', $uid, PDO::PARAM_INT);
      $r->execute();

      $stmt = "DELETE FROM $blacklist_table WHERE uid=:uid";
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
   global $conn, $user_table, $email_table, $whitelist_table, $blacklist_table, $misc_table, $err_sql_error, $err_existing_user, $BACK;

   while(list($k, $v) = each($_POST)) $$k = $v;

   $stmt = "INSERT INTO $user_table (uid, username, password, policy_group, isadmin) VALUES(:uid, :username, :password, :policy_group, :isadmin)";

   $c_pwd = crypt($password);

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':username', $username, PDO::PARAM_STR);
   $r->bindParam(':password', $c_pwd, PDO::PARAM_STR);
   $r->bindParam(':policy_group', $policy_group, PDO::PARAM_INT);
   $r->bindParam(':isadmin', $isadmin, PDO::PARAM_INT);

   $r->execute() or nice_error($err_existing_user . ". <a href=\"users.php\">$BACK.</a>");

   $stmt = "INSERT INTO $email_table (uid, email) VALUES(:uid, :email)";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':email', $email, PDO::PARAM_STR);
   $r->execute();



   $stmt = "INSERT INTO $whitelist_table (uid, whitelist) VALUES(:uid, :whitelist)";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':whitelist', $whitelist, PDO::PARAM_STR);
   $r->execute();

   $stmt = "INSERT INTO $blacklist_table (uid, blacklist) VALUES(:uid, :blacklist)";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':blacklist', $whitelist, PDO::PARAM_STR);
   $r->execute();

   $stmt = "INSERT INTO $misc_table (uid, nham, nspam) VALUES(:uid, 0, 0)";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->execute();

}


function update_user($uid){
   global $conn, $user_table, $email_table, $email_table, $whitelist_table, $blacklist_table, $err_sql_error;

   $PWD = "";

   while(list($k, $v) = each($_POST)) $$k = $v;

   if($password){
      $PWD = ", password=:password";
      $c_pwd = crypt($password);
   }

   $stmt = "UPDATE $user_table SET username=:username, policy_group=:policy_group, isadmin=:isadmin $PWD WHERE uid=:uid";

   $r = $conn->prepare($stmt);
   $r->bindParam(':username', $username, PDO::PARAM_STR);
   $r->bindParam(':policy_group', $policy_group, PDO::PARAM_INT);
   $r->bindParam(':isadmin', $isadmin, PDO::PARAM_INT);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   if($password) $r->bindParam(':password', $c_pwd, PDO::PARAM_STR);

   $r->execute() or nice_error($err_sql_error);


   $stmt = "UPDATE $email_table SET email=:email WHERE uid=:uid AND email=:email_orig";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':email', $email, PDO::PARAM_STR);
   $r->bindParam(':email_orig', $email_orig, PDO::PARAM_STR);

   $r->execute() or nice_error($err_sql_error);


   $stmt = "UPDATE $whitelist_table SET whitelist=:whitelist WHERE uid=:uid";
   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':whitelist', $whitelist, PDO::PARAM_STR);
   $r->execute() or nice_error($err_sql_error);

   $stmt = "UPDATE $blacklist_table SET blacklist=:blacklist WHERE uid=:uid";
   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':blacklist', $blacklist, PDO::PARAM_STR);
   $r->execute() or nice_error($err_sql_error);
}


function change_password(){
   global $user_table, $username, $err_user_failed_to_modify;

   $c_pwd = crypt($_POST['password']);

   $stmt = "UPDATE $user_table SET password=:password WHERE username=:username";

   $conn = webui_connect() or nice_error($err_connect_db);

   $r = $conn->prepare($stmt);
   $r->bindParam(':username', $username, PDO::PARAM_STR);
   $r->bindParam(':password', $c_pwd, PDO::PARAM_STR);

   $r->execute() or nice_error($err_sql_error);

   webui_close($conn);

   return 1;
}


function print_alias($x, $ro_username = 0){
   global $USERNAME, $EMAIL_ADDRESS;

   if($ro_username == 1)
      print "<tr><td>$USERNAME:</td><td>$x[0]</td></tr>\n";
   else
      print "<tr><td>$USERNAME:</td><td><input type=\"text\" name=\"username\" value=\"$x[0]\"></td></tr>\n";

   print "<tr><td>$EMAIL_ADDRESS:</td><td><input type=\"text\" name=\"email\" value=\"$x[1]\"></td></tr>\n";
}


function add_email_entry(){
   global $conn, $email_table, $err_sql_error, $err_existing_email, $BACK;

   while(list($k, $v) = each($_POST)) $$k = $v;

   $stmt = "INSERT INTO $email_table (uid, email) VALUES(:uid, :email)";

   $r = $conn->prepare($stmt);
   $r->bindParam(':uid', $uid, PDO::PARAM_INT);
   $r->bindParam(':email', $email, PDO::PARAM_STR);
   $r->execute();

}


/*** policy groups ***/


function show_existing_policy_groups($id = 0){
   global $policy_group_table, $err_sql_error, $REMOVE;
}

function get_policy_group_name_by_id($id){
   global $policy_group_table, $err_sql_error, $default_policy;

   if($id == 0) return $default_policy;

   return $name;
}

