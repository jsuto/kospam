<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

if($username == "") show_auth_popup();

include_once("header.php");

if($admin_user != 1) nice_error($err_you_are_not_admin);

$uid = -1;
$username = "";
$email = "";
$policy_group = 0;

$add = 0;
$edit = 0;
$remove = 0;
$modify = 0;
$bulk_edit = 0;


if(isset($_POST['uid'])) $uid = $_POST['uid'];
if(isset($_POST['username'])) $username = $_POST['username'];
if(isset($_POST['email'])) $email = $_POST['email'];
if(isset($_POST['policy_group'])) $policy_group = $_POST['policy_group'];
if(isset($_POST['modify'])) $modify = $_POST['modify'];
if(isset($_POST['add'])) $add = $_POST['add'];
if(isset($_POST['bulkedit'])) $bulkedit = $_POST['bulkedit'];
if(isset($_POST['remove'])) $remove = 1;

if(isset($_GET['uid'])) $uid = $_GET['uid'];
if(isset($_GET['email'])) $email = $_GET['email'];
if(isset($_GET['remove'])) $remove = 1;
if(isset($_GET['edit'])) $edit = $_GET['edit'];
if(isset($_GET['add'])) $add = $_GET['add'];


$conn = webui_connect() or nice_error($err_connect_db);


function create_uid_list(){
   $uidlist = "";

   reset($_POST);

   while(list($k, $v) = each($_POST)){
      if(preg_match("/^aa_/", $k)){
         $a = explode("_", $k);
         if(is_numeric($a[1]))
            $uidlist .= "$a[1],";
      }
   }

   $uidlist = preg_replace("/\,$/", "", $uidlist);

   return $uidlist;
}


?>


  <h3><?php print $USER_MANAGEMENT; ?></h3>



<p>


<?php

if($modify == 1){
   $uidlist = create_uid_list();

   bulk_update_user($uidlist);
   nice_screen("$err_modified_user. <a href=\"users.php\">$BACK.</a>");
}

if($remove == 1){
   $uidlist = create_uid_list();

   bulk_delete_user($uidlist);
   nice_screen("$err_removed_user_successfully. <a href=\"users.php\">$BACK.</a>");
}

else {

   $len = 30;
   $i = 0;

   $x = "<tr><td>uids: ";

   while(list($k, $v) = each($_POST)){
      if(preg_match("/^aa_/", $k)){
         //$x .= "<tr><td>" . preg_replace("/aa_/", "", $k) . "</td></tr>\n";
         $x .= preg_replace("/aa_/", "", $k) . ", ";
         if(preg_match("/^aa_/", $k)) $s .= "<input type=\"hidden\" name=\"$k\" value=\"1\">\n";
         $i++;
      }
   }

   if($i){
      $x = preg_replace("/\, $/", "", $x);
      $x .= "</td></tr>\n";

      print "<h4>$BULK_USER_UPDATE</h4>\n";
      print "<form action=\"massusers.php\" name=\"adduser\" method=\"post\">\n";
      print "<input type=\"hidden\" name=\"modify\" value=\"1\">\n";

      print $s;
      print "<table border=\"0\">\n";

      print "$x\n";

      print "<tr><td>$POLICY_GROUP:</td><td>\n";

      print "<select name=\"policy_group\">\n";
      print "<option value=\"0\">$default_policy</option>\n";
      show_existing_policy_groups(0);
      print "</select>\n";

      print "<tr valign=\"top\"><td>$WHITELIST:</td><td><textarea name=\"whitelist\" cols=\"$len\" rows=\"5\"></textarea></td></tr>\n";

      print "<tr valign=\"top\"><td>$BLACKLIST:</td><td><textarea name=\"blacklist\" cols=\"$len\" rows=\"5\"></textarea></td></tr>\n";

      print "</td></tr>\n";

      print "<tr colspan=\"2\"><td><input type=\"submit\" value=\"$UPDATE_SELECTED_UIDS\"></td></tr>\n";
      print "</table>\n";
      print "</form>\n";



      print "<h4>$REMOVE_SELECTED_UIDS</h4>\n";
      print "<form action=\"massusers.php\" name=\"removeuser\" method=\"post\">\n";
      print "<input type=\"hidden\" name=\"remove\" value=\"1\">\n";

      print "<table border=\"0\">\n";

      reset($_POST);

      while(list($k, $v) = each($_POST)){
         if(preg_match("/^aa_/", $k)){
            print "<input type=\"hidden\" name=\"$k\" value=\"1\">\n";
         }
      }

      print "$x\n";

      print "<tr colspan=\"2\"><td><input type=\"submit\" value=\"$REMOVE_SELECTED_UIDS\"></td></tr>\n";
      print "</table>\n";
      print "</form>\n";
   }
   else nice_error("$err_please_select_uids, <a href=\"users.php\">$BACK.</a>");
}


webui_close($conn);


?>


</p>


      </td>
      <td>
      </td>
    </tr>
   </table>






<?php include_once("footer.php"); ?>
