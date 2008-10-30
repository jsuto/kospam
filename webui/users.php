<?

include_once("config.php");
include_once("header.inc");

$username = $_SERVER['REMOTE_USER'];
if($username == "") nice_error($err_not_authenticated);

if($admin_user != 1) nice_error($err_you_are_not_admin);

$uid = -1;
$username = "";
$email = "";

$remove = 0;

if(isset($_POST['uid'])) $uid = $_POST['uid'];
if(isset($_POST['username'])) $username = $_POST['username'];
if(isset($_POST['email'])) $email = $_POST['email'];

if(isset($_GET['uid'])) $uid = $_GET['uid'];
if(isset($_GET['email'])) $email = $_GET['email'];
if(isset($_GET['remove'])) $remove = 1;

$conn = webui_connect() or nice_error($err_connect_db);

?>


  <h3><? print $USER_MANAGEMENT; ?></h3>

  <div id="body">


<h4><? print $ADD_NEW_USER; ?></h4>

<p>
<form action="users.php" name="adduser" method="post">
   <input type="hidden" name="add" value="1">
   <table border="0">
      <tr><td><? print $EMAIL_ADDRESS; ?>:</td><td><input type="text" name="email"></td></tr>
      <tr><td>Username:</td><td><input type="text" name="username" ></td></tr>
      <tr><td>User id:</td><td><input type="text" name="uid" ></td></tr>
      <tr colspan="2"><td><input type="submit" value="OK"></td></tr>
   </table>
</form>
</p>


<p>


<?

if($uid >= 0 && is_numeric($uid) && $email && $username){
   add_user_entry($uid, $username, $email);
   nice_screen($err_added_user_successfully . ". <a href=\"users.php\">$BACK.</a>");
}

else if($remove == 1 && $uid >= 1 && is_numeric($uid) && $email){
   /* remove the given email address/alias */

   delete_existing_user_entry($uid, $email);
   nice_screen($err_removed_user_successfully . ". <a href=\"users.php\">$BACK.</a>");

}

else {

   /* list current users/aliases */

   print "<h4>$EXISTING_USERS</h4>\n";

   print "<table border=\"1\">\n";
   print "<tr align=\"center\"><th>UID</th><th>$USERNAME</th><th>$EMAIL_ADDRESS</th><th>&nbsp;</th></tr>\n";

   show_existing_users();

   print "</table>\n";
}

webui_close($conn);


?>


</p>


      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<? include_once("footer.inc"); ?>
