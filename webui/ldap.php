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
   $justthese = array("uid", "mail", "cn");

   $sr = ldap_search($conn, $basedn, $filter, $justthese);

   $info = ldap_get_entries($conn, $sr);

   for($i=0; $i<$info["count"]; $i++){
      $uid = $info[$i]["uid"][0];
      $username = $info[$i]["cn"][0];
      $email = $info[$i]["mail"][0];

      print "<tr><td>$uid</td><td>$username</td><td>$email</td><td><a href=\"users.php?remove=1&uid=$uid&email=$email\">$REMOVE</a></td></tr>\n";
   }

}


function delete_existing_user_entry($uid, $email){
}


function add_user_entry($u, $user, $mail){
}

?>
