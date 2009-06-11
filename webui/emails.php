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
$password2 = "";
$page = 0;
$add = 0;
$edit = 0;
$remove = 0;
$modify = 0;
$alias = 0;

$what = "";

if(isset($_POST['uid'])) $uid = $_POST['uid'];
if(isset($_POST['username'])) $username = $_POST['username'];
if(isset($_POST['email'])) $email = $_POST['email'];
if(isset($_POST['policy_group'])) $policy_group = $_POST['policy_group'];
if(isset($_POST['password'])) $password = $_POST['password'];
if(isset($_POST['password2'])) $password2 = $_POST['password2'];
if(isset($_POST['modify'])) $modify = $_POST['modify'];
if(isset($_POST['add'])) $add = $_POST['add'];
if(isset($_POST['what'])) $what = $_POST['what'];

if(isset($_GET['uid'])) $uid = $_GET['uid'];
if(isset($_GET['username'])) $username = $_GET['username'];
if(isset($_GET['email'])) $email = $_GET['email'];
if(isset($_GET['remove'])) $remove = 1;
if(isset($_GET['edit'])) $edit = $_GET['edit'];
if(isset($_GET['add'])) $add = $_GET['add'];
if(isset($_GET['alias'])) $alias = $_GET['alias'];
if(isset($_GET['page'])) $page = $_GET['page'];
if(isset($_GET['what'])) $what = $_GET['what'];

$conn = webui_connect() or nice_error($err_connect_db);

?>


  <h3><?php print $USER_MANAGEMENT; ?></h3>



<p>


<?php

if($add == 1 && $uid >= 0 && is_numeric($uid) && $email){
   add_email_entry($uid);
   nice_screen($err_added_email_successfully . ". <a href=\"users.php\">$BACK.</a>");
}

else if($add == 1){
   print "<h4>$ADD_NEW_EMAIL</h4>\n";
   print "<form action=\"emails.php\" name=\"adduser\" method=\"post\">\n";
   print "<input type=\"hidden\" name=\"add\" value=\"1\">\n";
   print "<input type=\"hidden\" name=\"uid\" value=\"$uid\">\n";
   print "<table border=\"0\">\n";

   $x = array($username, '');

   print_alias($x, 1);

   print "<tr colspan=\"2\"><td><input type=\"submit\" value=\"$ADD\"></td></tr>\n";
   print "</table>\n";
   print "</form>\n";

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
