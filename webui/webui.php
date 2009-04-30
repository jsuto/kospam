<?php


function get_authenticated_username(){
   global $userdb;

   $x = 0;

   if(isset($_SESSION['username'])) return $_SESSION['username'];

   if(isset($_SERVER['PHP_AUTH_USER']) && isset($_SERVER['PHP_AUTH_PW']) && !empty($_SESSION['auth'])){

      /* check whether the given username + password is ok */

      $x = check_user_auth();

      if($x == 1){
         $_SESSION['username'] = $_SERVER['PHP_AUTH_USER'];
         return $_SERVER['PHP_AUTH_USER'];
      }
   }
   else return "";
}


function is_admin(){
   global $userdb;

   $x = 0;

   if(!isset($_SESSION['username'])) return $x;

   $x = is_admin_user();

   return $x;
}


function show_auth_popup(){
   global $err_not_authenticated;

   header('WWW-Authenticate: Basic Realm="webui - login please"');
   header('HTTP/1.0 401 Unauthorized');
   $_SESSION['auth'] = true;
   nice_error($err_not_authenticated);
}


function logout(){
   $_SESSION['auth'] = null;
   $_SESSION['username'] = "";
   session_destroy();
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
   print "<tr><td>store_metadata</td><td><input name=\"store_metadata\" value=\"$x[19]\" size=\"3\"></td><td>0|1</td></tr>\n";

}



