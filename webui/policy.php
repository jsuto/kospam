<?

include_once("config.php");
include_once("header.inc");

$add = 0;
$view = 0;
$modify = 0;
$remove = 0;

$policy_group = 0;

if(isset($_GET['add'])) $add = $_GET['add'];
if(isset($_GET['view'])) $view = $_GET['view'];
if(isset($_GET['remove'])) $remove = $_GET['remove'];

if(isset($_GET['policy_group'])) $policy_group = $_GET['policy_group'];

if(isset($_POST['add'])) $add = $_POST['add'];
if(isset($_POST['modify'])) $modify = $_POST['modify'];
if(isset($_POST['policy_group'])) $policy_group = $_POST['policy_group'];
if(isset($_POST['name'])) $name = $_POST['name'];


$username = $_SERVER['REMOTE_USER'];
if($username == "") nice_error($err_not_authenticated);

$conn = webui_connect() or nice_error($err_connect_db);

?>


  <h3><? print $POLICY; ?></h3>

  <div id="body">

<p>

<?

if($modify == 1 && is_numeric($policy_group) && $policy_group > 0){
   update_policy($policy_group);

   nice_screen("$err_modified_policy. <a href=\"policy.php\">$BACK.</a>");
}

else if($view == 1 && is_numeric($policy_group) && $policy_group >= 0){
   if($policy_group == 0) nice_error("$err_cannot_view_default_policy. <a href=\"policy.php\">$BACK.</a>");

   print "<form action=\"policy.php\" method=\"post\">\n";
   print "<input type=\"hidden\" name=\"policy_group\" value=\"$policy_group\">\n";
   print "<input type=\"hidden\" name=\"modify\" value=\"1\">\n";

   show_policy($policy_group);

   print "</form>\n\n\n";

   print "<p>&nbsp;</p>\n<p><a href=\"policy.php?policy_group=$policy_group&remove=1\">$REMOVE_POLICY</a></p>\n";

   print "<p>&nbsp;</p>\n<a href=\"policy.php\">$BACK.</a>\n";
}

else if($remove == 1 && is_numeric($policy_group) && $policy_group >= 0){
   $stmt = "DELETE FROM $policy_group_table WHERE policy_group=$policy_group";
   mysql_query($stmt) or nice_error($err_sql_error);
   nice_screen("$err_removed_policy. <a href=\"policy.php\">$BACK.</a>");
}

else if($add == 1 && $name){
   add_policy();

   nice_screen("$err_added_new_policy. <a href=\"policy.php\">$BACK.</a>");
}

else if($add == 1){
   $x = array('', 0, 1, 1, '', 1, 128000, '', '', 0.92, 1.01, 1, 5, 1, 0, 0, 0, 0, 0);

   print "<form action=\"policy.php\" method=\"post\">\n";
   print "<input type=\"hidden\" name=\"add\" value=\"1\">\n";

   print "<table>\n";

   print_policy($x);

   print "<tr><td><input type=\"submit\" value=\"$ADD\"> <input type=\"reset\" value=\"$CANCEL\"></td><td></td></tr>\n";
   print "</table>\n";

   print "</form>\n\n\n";

   print "<p>&nbsp;</p>\n<a href=\"policy.php\">$BACK.</a>\n";
}

else {

   /* list current policies */

   print "<h4>$EXISTING_POLICY</h4>\n";

   print "<form action=\"policy.php\" method=\"get\">\n";
   print "<input type=\"hidden\" name=\"view\" value=\"1\">\n";
   print "<select name=\"policy_group\">\n";
   print "<option value=\"0\">$default_policy</option>\n";
   show_existing_policy_groups();
   print "</select>\n";

   print "<input type=\"submit\" value=\"$EDIT_OR_VIEW\">\n";


   //print "<h4>$ADD_NEW_POLICY</h4>\n";

   print "<p>&nbsp;</p>\n<a href=\"policy.php?add=1\">$ADD_NEW_POLICY</a>\n";
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
