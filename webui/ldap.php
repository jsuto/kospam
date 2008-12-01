<?

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


function show_existing_users(){
   global $basedn, $conn, $REMOVE;

   $filter="cn=*";
   $justthese = array("uid", "mail", "cn", "policygroupid");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);

   $info = ldap_get_entries($conn, $sr);

   //print_r($info);

   for($i=0; $i<$info["count"]; $i++){
      $uid = $info[$i]["uid"][0];
      $username = $info[$i]["cn"][0];
      $email = $info[$i]["mail"][0];
      $policy_group = $info[$i]["policygroupid"][0];

      //if($uid == 2000) print_r($info[$i]);

      if($policy_group == "") $policy_group = 0;
      $policy_group = get_policy_group_name_by_id($policy_group);
      print "<tr><td>$uid</td><td>$username</td><td>$email</td><td>$policy_group</td><td><a href=\"users.php?remove=1&uid=$uid&email=$email\">$REMOVE</a></td></tr>\n";
   }

}


function delete_existing_user_entry($uid, $email){
}


function add_user_entry($u, $user, $mail){
}


/*** policy groups ***/

function get_policy_group_name_by_id($id){
   global $basedn, $conn, $default_policy;

   $name = "";

   if($id == 0) return $default_policy;

   $filter="policyGroup=$id";
   $justthese = array("policyName");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);
   $info = ldap_get_entries($conn, $sr);

   $name = $info[0]["policyname"][0];

   return $name;
}


function get_new_policy_group_id(){
   global $basedn, $conn;

   $x = 0;

   $filter="policyGroup=*";
   $justthese = array("policyGroup");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);
   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      $policy_group = $info[$i]["policygroup"][0];
      if($policy_group > $x) $x = $policy_group;
   }

   return $x + 1;
}


function show_existing_policy_groups($id = 0){
   global $basedn, $conn;

   $filter="policyGroup=*";
   $justthese = array("policyGroup", "policyName");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);
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
   global $basedn, $conn, $POLICY, $MODIFY, $CANCEL;

   $x = array();

   $filter="policyGroup=$policy_group";
   $justthese = array("policyName", "deliverinfectedemail", "silentlydiscardinfectedemail", "useantispam", "spamsubjectprefix", "enableautowhitelist", "maxmessagesizetofilter", "rbldomain", "surbldomain", "spamoveralllimit", "spaminessoblivionlimit", "replacejunkcharacters", "invalidjunklimit", "invalidjunkline", "penalizeimages", "penalizeembedimages", "penalizeoctetstream", "trainingmode", "initial1000learning");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);
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
   global $basedn, $conn;
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


   $policy_group = get_new_policy_group_id();

   $dn = "policyGroup=$policy_group,ou=clapfpolicies,$basedn";

   print_r($entry);

   print "<p>\n$dn\n";

   $sr = ldap_add($conn, $dn, $entry);
   print ldap_error($conn);

}


?>
