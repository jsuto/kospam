<?php

function webui_connect(){
   global $ldaphost, $binddn, $bindpw, $err_connect_db;

   $ldapconn = ldap_connect($ldaphost) or nice_error($err_connect_db);
   ldap_set_option($ldapconn, LDAP_OPT_PROTOCOL_VERSION, 3);

   if(!ldap_bind($ldapconn, $binddn, $bindpw)) nice_error($err_connect_db);

   return $ldapconn;
}


function webui_close($conn){
   ldap_unbind($conn);
}


function check_user_auth($username, $password){
   global $ldaphost, $user_base_dn, $err_connect_db;

   $binddn = "cn=$username,$user_base_dn";
   $bindpw = $password;

   $ldapconn = ldap_connect($ldaphost) or nice_error($err_connect_db);
   ldap_set_option($ldapconn, LDAP_OPT_PROTOCOL_VERSION, 3);

   if(!ldap_bind($ldapconn, $binddn, $bindpw)) return 0;

   ldap_unbind($ldapconn);

   return 1;
}


function is_admin_user($username){
   global $basedn, $user_base_dn;

   $x = 0;

   $filter="cn=$username";
   $justthese = array("isadmin");

   $conn = webui_connect() or nice_error($err_connect_db);

   $sr = ldap_search($conn, $user_base_dn, $filter, $justthese);

   $info = ldap_get_entries($conn, $sr);

   if($info){
      $x = $info[0]["isadmin"][0];
   }

   webui_close($conn);
   return $x;
}


function print_email_addresses_for_user($conn, $username){
   global $basedn;

   $filter="cn=$username";
   $justthese = array("mail", "mailAlternateAddress");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);

   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      $c = $info[$i]["count"];
      for($j=0; $j<$c; $j++){
         $attr_name = $info[$i][$j];
         $c2 = $info[$i][$attr_name]["count"];
         for($k=0; $k<$c2; $k++){
            print "<tr><td>" . $info[$i][$attr_name][$k] . "</td></tr>\n";
         }
      }
   }
}


function get_whitelist_by_name($username){
   global $basedn, $conn;
   $whitelist = "";

   $filter="cn=$username";
   $justthese = array("filtersender");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);

   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      $c = $info[$i]["count"];
      for($j=0; $j<$c; $j++){
         $attr_name = $info[$i][$j];
         $c2 = $info[$i][$attr_name]["count"];
         for($k=0; $k<$c2; $k++){
            $whitelist = $info[$i][$attr_name][$k];
            break;
         }
      }
   }

   return $whitelist;
}


function get_db_by_name($username){
   global $basedn, $conn;
   $dn = "";

   $filter="cn=$username";
   $justthese = array("filtersender");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);
   $info = ldap_get_entries($conn, $sr);

   $dn = $info[0]["dn"];

   return $dn;
}


function set_whitelist($whitelist, $username){
   global $basedn, $conn;
   $entry = array();

   $dn = get_db_by_name($username);

   $entry["filtersender"] = $whitelist;

   ldap_mod_replace($conn, $dn, $entry);

}


function get_users_email_address($username){
   global $basedn, $conn;
   $to = "";

   $filter="cn=$username";
   $justthese = array("mail");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);

   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      $c = $info[$i]["count"];
      for($j=0; $j<$c; $j++){
         $attr_name = $info[$i][$j];
         $c2 = $info[$i][$attr_name]["count"];
         for($k=0; $k<$c2; $k++){
	    $to = $info[$i][$attr_name][$k];
            break;
         }
      }
   }

   return $to;
}


function get_next_uid(){
   global $basedn, $conn;
   $uid = 1;

   $filter="uid=*";
   $justthese = array("uid");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);

   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      $__uid = $info[$i]["uid"][0];

      if($__uid > $uid) $uid = $__uid;
   }

   $uid++;

   return $uid;
}


function show_existing_users($what, $page, $page_len){
   global $basedn, $conn, $EDIT_OR_VIEW;
   $n_users = 0;
   $from = $page * $page_len;
   $to = ($page+1) * $page_len;

   $filter="(|(cn=$what*)(mail=$what*))";

   $justthese = array("uid", "mail", "cn", "policygroupid");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);

   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      $uid = $info[$i]["uid"][0];
      $username = $info[$i]["cn"][0];
      $email = $info[$i]["mail"][0];
      $policy_group = $info[$i]["policygroupid"][0];

      if($policy_group == "") $policy_group = 0;
      $policy_group = get_policy_group_name_by_id($policy_group);

      if($n_users >= $from && $n_users < $to)
         print "<tr align=\"left\"><td><input type=\"checkbox\" name=\"aa_$uid\"></td><td>$uid</td><td>$username</td><td>$email</td><td>$policy_group</td><td><a href=\"users.php?uid=$uid&email=$email&edit=1\">$EDIT_OR_VIEW</a></td></tr>\n";

      $n_users++;
   }

   return $n_users;
}


function print_user($x, $ro_uid = 0){
   global $EMAIL_ADDRESS, $USERNAME, $PASSWORD, $USERID, $POLICY_GROUP, $ADMIN_USER, $ALIASES, $WHITELIST, $BLACKLIST, $default_policy;

   $len = 30;
   $aliases = "";
   $whitelist = "";

   print "<input type=\"hidden\" name=\"cn\" value=\"$x[1]\">\n";

   print "<tr><td>$USERNAME:</td><td><input type=\"text\" name=\"username\" value=\"$x[1]\" size=\"$len\"></td></tr>\n";
   print "<tr><td>$PASSWORD:</td><td><input type=\"password\" name=\"password\" value=\"\"></td></tr>\n";

   if($ro_uid == 1)
      print "<tr><td>$USERID:</td><td>$x[2]</td></tr>\n";
   else
      print "<tr><td>$USERID:</td><td><input type=\"text\" name=\"uid\" value=\"$x[2]\" size=\"$len\"></td></tr>\n";

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

   print "<tr><td>$EMAIL_ADDRESS:</td><td><input type=\"text\" name=\"email\" value=\"$x[0]\" size=\"$len\"></td></tr>\n";

   if($x[5]){
      array_shift($x[5]);
      while(list($k, $v) = each($x[5])) $aliases .= "$v\n";
      $aliases = preg_replace("/\n$/", "", $aliases);
   }

   print "<tr valign=\"top\"><td>$ALIASES:</td><td><textarea name=\"mailAlternateAddress\" cols=\"$len\" rows=\"5\">$aliases</textarea></td></tr>\n";

   if($x[6]){
      array_shift($x[6]);
      while(list($k, $v) = each($x[6])) $whitelist .= "$v\n";
      $whitelist = preg_replace("/\n$/", "", $whitelist);
   }

   print "<tr valign=\"top\"><td>$WHITELIST:</td><td><textarea name=\"filtersender\" cols=\"$len\" rows=\"5\">$whitelist</textarea></td></tr>\n";

   if($x[7]){
      array_shift($x[7]);
      while(list($k, $v) = each($x[6])) $blacklist .= "$v\n";
      $whitelist = preg_replace("/\n$/", "", $blacklist);
   }

   print "<tr valign=\"top\"><td>$BLACKLIST:</td><td><textarea name=\"blacklist\" cols=\"$len\" rows=\"5\">$blacklist</textarea></td></tr>\n";
}


function delete_existing_user_entry($uid, $email){
   global $basedn, $conn, $user_base_dn,$err_user_failed_to_remove;

   $x = get_user_entry($uid);

   $dn = "cn=$x[1],$user_base_dn";
   ldap_delete($conn, $dn) or nice_error($err_user_failed_to_remove);
}


function get_user_entry($uid){
   global $basedn, $conn, $user_base_dn;

   $a = array();

   $filter="uid=$uid";
   $justthese = array("uid", "cn", "sn", "mail", "filtersender", "mailMessageStore", "mailAlternateAddress", "policygroupid", "isAdmin");

   $sr = ldap_search($conn, $user_base_dn, $filter, $justthese);

   $info = ldap_get_entries($conn, $sr);

   if($info){
      $a[0] = $info[0]["mail"][0];
      $a[1] = $info[0]["cn"][0];
      $a[2] = $info[0]["uid"][0];
      $a[3] = $info[0]["policygroupid"][0];
      $a[4] = $info[0]["isadmin"][0];
      $a[5] = $info[0]["mailalternateaddress"];
      $a[6] = $info[0]["filtersender"];
      $a[7] = $info[0]["blacklist"];
   }

   return $a;
}


function add_user_entry($uid){
   global $basedn, $conn, $user_base_dn, $err_existing_user, $err_failed_to_add_user, $BACK;

   $entry = array();
   $a = array();
   $a[0] = "top";
   $a[1] = "person";
   $a[2] = "qmailUser";
   $a[3] = "qmailGroup";

   $c = trim_to_array($_POST['mailAlternateAddress']);
   $b = trim_to_array($_POST['filtersender']);
   $d = trim_to_array($_POST['blacklist']);

   $user = $_POST['username'];

   $entry["objectClass"] = $a;
   $entry["cn"] = $user;
   $entry["sn"] = "x";
   $entry["mail"] = $_POST['email'];
   $entry["uid"] = $uid;

   if(count($b) > 0) $entry["filtersender"] = $b;
   else $entry["filtersender"] = "";

   $entry["mailMessageStore"] = "";

   if(count($c) > 0) $entry["mailAlternateAddress"] = $c;
   else $entry["mailAlternateAddress"] = "";

   if(count($d) > 0) $entry["blacklist"] = $d;
   else $entry["blacklist"] = "";

   $entry["policyGroupId"] = $_POST['policy_group'];
   $entry["userPassword"] = $_POST['password'];
   $entry["isAdmin"] = $_POST["isadmin"];


   $dn = "cn=$user,$user_base_dn";

   $sr = ldap_add($conn, $dn, $entry) or nice_error($err_failed_to_add_user);
}


function update_user($uid){
   global $basedn, $conn, $user_base_dn, $err_user_failed_to_modify;

   $entry = array();

   $a = trim_to_array($_POST['mailAlternateAddress']);
   $b = trim_to_array($_POST['filtersender']);
   $c = trim_to_array($_POST['blacklist']);

   $entry["cn"] = $_POST['username'];
   $entry["mail"] = $_POST['email'];
   $entry["policyGroupId"] = $_POST['policy_group'];
   $entry["mailAlternateAddress"] = $a;
   $entry["filtersender"] = $b;
   $entry["blacklist"] = $c;

   if(isset($_POST['password']) && strlen($_POST['password']) > 3) $entry["userPassword"] = $_POST['password'];

   $entry["isAdmin"] = $_POST['isadmin'];

   $dn = "cn=" . $_POST['cn'] . ",$user_base_dn";

   $sr = ldap_modify($conn, $dn, $entry) or nice_error($err_user_failed_to_modify);

}


function change_password(){
   global $basedn, $user_base_dn, $username, $err_user_failed_to_modify;

   $entry = array();

   $entry["cn"] = $username;
   $entry["userPassword"] = $_POST['password'];

   $dn = "cn=" . $username . ",$user_base_dn";

   $conn = webui_connect() or nice_error($err_connect_db);
   $sr = ldap_modify($conn, $dn, $entry) or nice_error($err_user_failed_to_modify);
   webui_close($conn);

   return 1;
}


/*** policy groups ***/

function get_policy_group_name_by_id($id){
   global $basedn, $conn, $policy_base_dn, $default_policy;

   $name = "";

   if($id == 0) return $default_policy;

   $filter="policyGroup=$id";
   $justthese = array("policyName");

   $sr = ldap_search($conn, $policy_base_dn, $filter, $justthese);
   $info = ldap_get_entries($conn, $sr);

   $name = $info[0]["policyname"][0];

   return $name;
}


function get_new_policy_group_id(){
   global $basedn, $conn, $policy_base_dn;

   $x = 0;

   $filter="policyGroup=*";
   $justthese = array("policyGroup");

   $sr = ldap_search($conn, $policy_base_dn, $filter, $justthese);
   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      $policy_group = $info[$i]["policygroup"][0];
      if($policy_group > $x) $x = $policy_group;
   }

   return $x + 1;
}


function show_existing_policy_groups($id = 0){
   global $basedn, $conn, $policy_base_dn;

   $filter="policyGroup=*";
   $justthese = array("policyGroup", "policyName");

   $sr = ldap_search($conn, $policy_base_dn, $filter, $justthese);
   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      $policy_group = $info[$i]["policygroup"][0];
      $name = $info[$i]["policyname"][0];

      if($policy_group == $id)
         print "<option value=\"$policy_group\" selected>$name</option>\n";
      else
         print "<option value=\"$policy_group\">$name</option>\n";
   }
}


function show_policy($policy_group){
   global $basedn, $conn, $policy_base_dn, $POLICY, $MODIFY, $CANCEL;

   $x = array();

   $filter="policyGroup=$policy_group";
   $justthese = array("policyName", "deliverinfectedemail", "silentlydiscardinfectedemail", "useantispam", "spamsubjectprefix", "enableautowhitelist", "maxmessagesizetofilter", "rbldomain", "surbldomain", "spamoveralllimit", "spaminessoblivionlimit", "replacejunkcharacters", "invalidjunklimit", "invalidjunkline", "penalizeimages", "penalizeembedimages", "penalizeoctetstream", "trainingmode", "initial1000learning");

   $sr = ldap_search($conn, $policy_base_dn, $filter, $justthese);
   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      //print_r($info[$i]);

      $x[0] = $info[$i]["policyname"][0];
      $x[1] = $info[$i]["deliverinfectedemail"][0];
      $x[2] = $info[$i]["silentlydiscardinfectedemail"][0];
      $x[3] = $info[$i]["useantispam"][0];
      $x[4] = $info[$i]["spamsubjectprefix"][0];
      $x[5] = $info[$i]["enableautowhitelist"][0];
      $x[6] = $info[$i]["maxmessagesizetofilter"][0];
      $x[7] = $info[$i]["rbldomain"][0];
      $x[8] = $info[$i]["surbldomain"][0];
      $x[9] = $info[$i]["spamoveralllimit"][0];
      $x[10] = $info[$i]["spaminessoblivionlimit"][0];
      $x[11] = $info[$i]["replacejunkcharacters"][0];
      $x[12] = $info[$i]["invalidjunklimit"][0];
      $x[13] = $info[$i]["invalidjunkline"][0];
      $x[14] = $info[$i]["penalizeimages"][0];
      $x[15] = $info[$i]["penalizeembedimages"][0];
      $x[16] = $info[$i]["penalizeoctetstream"][0];
      $x[17] = $info[$i]["trainingmode"][0];
      $x[18] = $info[$i]["initial1000learning"][0];

   }

   print "<table>\n";

   print_policy($x);

   print "<tr><td><input type=\"submit\" value=\"$MODIFY\"> <input type=\"reset\" value=\"$CANCEL\"></td><td></td></tr>\n";
   print "</table>\n";

}



function add_policy(){
   global $basedn, $conn, $policy_base_dn, $err_policy_failed_to_add;
   $entry = array();

   $policy_group = get_new_policy_group_id();

   $entry["objectClass"] = "clapfPolicyGroup";
   $entry["policyGroup"] = $policy_group;
   $entry["policyname"] = $_POST['name'];
   $entry["deliverinfectedemail"] = $_POST['deliver_infected_email'];
   $entry["silentlydiscardinfectedemail"] = $_POST['silently_discard_infected_email'];
   $entry["useantispam"] = $_POST['use_antispam'];
   $entry["spamsubjectprefix"] = $_POST['spam_subject_prefix'];
   $entry["enableautowhitelist"] = $_POST['enable_auto_white_list'];
   $entry["maxmessagesizetofilter"] = $_POST['max_message_size_to_filter'];
   $entry["rbldomain"] = $_POST['rbl_domain'];
   $entry["surbldomain"] = $_POST['surbl_domain'];
   $entry["spamoveralllimit"] = $_POST['spam_overall_limit'];
   $entry["spaminessoblivionlimit"] = $_POST['spaminess_oblivion_limit'];
   $entry["replacejunkcharacters"] = $_POST['replace_junk_characters'];
   $entry["invalidjunklimit"] = $_POST['invalid_junk_limit'];
   $entry["invalidjunkline"] = $_POST['invalid_junk_line'];
   $entry["penalizeimages"] = $_POST['penalize_images'];
   $entry["penalizeembedimages"] = $_POST['penalize_embed_images'];
   $entry["penalizeoctetstream"] = $_POST['penalize_octet_stream'];
   $entry["trainingmode"] = $_POST['training_mode'];
   $entry["initial1000learning"] = $_POST['initial_1000_learning'];


   $dn = "policyGroup=$policy_group,$policy_base_dn";

   $sr = ldap_add($conn, $dn, $entry) or nice_error($err_policy_failed_to_add);
}


function update_policy($policy_group){
   global $basedn, $conn, $policy_base_dn, $err_policy_failed_to_modify;

   $entry = array();

   $entry["policyname"] = $_POST['name'];
   $entry["deliverinfectedemail"] = $_POST['deliver_infected_email'];
   $entry["silentlydiscardinfectedemail"] = $_POST['silently_discard_infected_email'];
   $entry["useantispam"] = $_POST['use_antispam'];
   $entry["spamsubjectprefix"] = $_POST['spam_subject_prefix'];
   $entry["enableautowhitelist"] = $_POST['enable_auto_white_list'];
   $entry["maxmessagesizetofilter"] = $_POST['max_message_size_to_filter'];
   $entry["rbldomain"] = $_POST['rbl_domain'];
   $entry["surbldomain"] = $_POST['surbl_domain'];
   $entry["spamoveralllimit"] = $_POST['spam_overall_limit'];
   $entry["spaminessoblivionlimit"] = $_POST['spaminess_oblivion_limit'];
   $entry["replacejunkcharacters"] = $_POST['replace_junk_characters'];
   $entry["invalidjunklimit"] = $_POST['invalid_junk_limit'];
   $entry["invalidjunkline"] = $_POST['invalid_junk_line'];
   $entry["penalizeimages"] = $_POST['penalize_images'];
   $entry["penalizeembedimages"] = $_POST['penalize_embed_images'];
   $entry["penalizeoctetstream"] = $_POST['penalize_octet_stream'];
   $entry["trainingmode"] = $_POST['training_mode'];
   $entry["initial1000learning"] = $_POST['initial_1000_learning'];

   $dn = "policyGroup=$policy_group,$policy_base_dn";

   $sr = ldap_modify($conn, $dn, $entry) or nice_error($err_policy_failed_to_modify);

}


function remove_policy($policy_group){
   global $basedn, $conn, $policy_base_dn,$err_policy_failed_to_remove;

   $dn = "policyGroup=$policy_group,$policy_base_dn";
   ldap_delete($conn, $dn) or nice_error($err_policy_failed_to_remove);

}


?>
