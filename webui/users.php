<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

if($username == "") show_auth_popup();

include_once("header.php");

if($admin_user != 1) nice_error($err_you_are_not_admin);

if(isset($_COOKIE['pagelen'])){
   if($_COOKIE['pagelen'] >= 10 && $_COOKIE['pagelen'] <= 50) $page_len = $_COOKIE['pagelen'];
}

$meurl = $_SERVER['PHP_SELF'];

$uid = -1;
$username = "";
$email = "";
$policy_group = 0;
$password = "";

$page = 0;
$add = 0;
$edit = 0;
$remove = 0;
$modify = 0;

$what = "";

if(isset($_POST['uid'])) $uid = $_POST['uid'];
if(isset($_POST['username'])) $username = $_POST['username'];
if(isset($_POST['email'])) $email = $_POST['email'];
if(isset($_POST['policy_group'])) $policy_group = $_POST['policy_group'];
if(isset($_POST['password'])) $password = $_POST['password'];
if(isset($_POST['modify'])) $modify = $_POST['modify'];
if(isset($_POST['add'])) $add = $_POST['add'];
if(isset($_POST['what'])) $what = $_POST['what'];

if(isset($_GET['uid'])) $uid = $_GET['uid'];
if(isset($_GET['email'])) $email = $_GET['email'];
if(isset($_GET['remove'])) $remove = 1;
if(isset($_GET['edit'])) $edit = $_GET['edit'];
if(isset($_GET['add'])) $add = $_GET['add'];
if(isset($_GET['page'])) $page = $_GET['page'];
if(isset($_GET['what'])) $what = $_GET['what'];

$conn = webui_connect() or nice_error($err_connect_db);

?>


  <h3><?php print $USER_MANAGEMENT; ?></h3>



<p>


<?php

if($add == 1 && uid >= 0 && is_numeric($uid) && $email && $username){
   add_user_entry($uid); //, $username, $email, $policy_group);
   nice_screen($err_added_user_successfully . ". <a href=\"users.php\">$BACK.</a>");
}

else if($modify == 1 && $uid >= 0 && is_numeric($uid) && $email && $username){
   update_user($uid);
   nice_screen("$err_modified_user. <a href=\"users.php\">$BACK.</a>");
}

else if($edit == 1 && $uid >= 1 && is_numeric($uid)){
   $x = get_user_entry($uid, $email);

   print "<form action=\"users.php\" name=\"modifyuser\" method=\"post\">\n";
   print "<input type=\"hidden\" name=\"modify\" value=\"1\">\n";
   print "<input type=\"hidden\" name=\"uid\" value=\"$uid\">\n";
   print "<input type=\"hidden\" name=\"email_orig\" value=\"$x[0]\">\n";

   print "<table border=\"0\">\n";

   print_user($x, 1);

   print "<tr><td>&nbsp;</td><td><input type=\"submit\" value=\"$MODIFY\"> <input type=\"reset\" value=\"$CANCEL\"></td></tr>\n";
   print "</table>\n";
   print "</form>\n";

   print "<p>&nbsp;</p>\n<p><a href=\"users.php?uid=$uid&email=$x[0]&remove=1\">$REMOVE_USER</a></p>\n";

   print "<p>&nbsp;</p>\n<a href=\"users.php\">$BACK.</a>\n";
}

else if($add == 1){
   print "<h4>$ADD_NEW_USER</h4>\n";
   print "<form action=\"users.php\" name=\"adduser\" method=\"post\">\n";
   print "<input type=\"hidden\" name=\"add\" value=\"1\">\n";
   print "<table border=\"0\">\n";

   $next_uid = get_next_uid();

   $x = array('', '', $next_uid, 0, '', '', 0);
   print_user($x);

   print "<tr colspan=\"2\"><td><input type=\"submit\" value=\"$ADD\"></td></tr>\n";
   print "</table>\n";
   print "</form>\n";

}

else if($remove == 1 && $uid >= 1 && is_numeric($uid)){
   /* remove the given email address/alias */

   delete_existing_user_entry($uid, $email);
   nice_screen($err_removed_user_successfully . ". <a href=\"users.php\">$BACK.</a>");

}

else {

   /* list current users/aliases */

   print "<h4>$EXISTING_USERS</h4>\n";

   print "<form method=\"post\" name=\"search1\" action=\"users.php\">\n";
   print "<input type=\"text\" name=\"what\" value=\"$what\">\n";
   print "<input type=\"submit\" value=\"search\"></form><p/>\n";

   print "<form method=\"post\" name=\"massedit\" action=\"massusers.php\">\n";
   print "<input type=\"hidden\" name=\"bulkedit\" value=\"1\">\n";
   print "<table border=\"1\">\n";
   print "<tr align=\"center\"><th>&nbsp;</th><th>UID</th><th>$USERNAME</th><th>$EMAIL_ADDRESS</th><th>$POLICY_GROUP</th><th>&nbsp;</th></tr>\n";

   $n_users = show_existing_users($what, $page, $page_len);

   print "</table><p/>\n";

   /* paging */

      print "<p>\n";
      $prev_page = $page - 1;
      $next_page = $page + 1;
      $total_pages = floor($n_users/$page_len);

      if($page > 0)
         print "<a href=\"$meurl?page=0&what=$what\">$FIRST</a> <a href=\"$meurl?page=$prev_page&what=$what\">$PREVIOUS</a>\n";

      if($n_users >= $page_len*($page+1) && $n_users > $page_len)
         print " <a href=\"$meurl?page=$next_page&what=$what\">$NEXT</a>\n";

      if($page+1 < $n_users/$page_len)
 //&& $n_users > $page_len)
         print " <a href=\"$meurl?page=$total_pages&what=$what\">$LAST</a>\n";

      print "</p>\n";


   print "<input type=\"submit\" value=\"$BULK_EDIT_SELECTED_UIDS\"></form>\n";

   print "<p><a href=\"users.php?add=1\">$ADD_NEW_USER</a></p>\n";

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
