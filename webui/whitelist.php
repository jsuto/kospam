<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

include_once("header.php");

if($username == "") show_auth_popup();

$modify = 0;
$whitelist = "";

if(isset($_POST['modify'])) $modify = $_POST['modify'];
if(isset($_POST['whitelist'])) $whitelist = $_POST['whitelist'];

$conn = webui_connect() or nice_error($err_connect_db);

?>


  <h3><?php print $WHITELIST_SETTINGS; ?></h3>

  <div id="body">

<p>

<?php

if($modify == 1){
   set_whitelist($whitelist, $username);
   nice_screen("$err_successfully_modified. <a href=\"whitelist.php\">$BACK.</a>");
}

else {
   $whitelist = get_whitelist_by_name($username);

   print "<form name=\"settings\" action=\"whitelist.php\" method=\"post\">\n";
   print "<input type=\"hidden\" name=\"modify\" value=\"1\">\n";
   print "<table border=\"0\">\n";

   print "<tr valign=\"top\"><td><textarea name=\"whitelist\" cols=\"30\" rows=\"10\">$whitelist</textarea></td></tr>\n";

   print "<tr valign=\"top\"><td><input type=\"submit\" value=\"OK\"> <input type=\"reset\" value=\"$CANCEL\"></td></tr>\n";


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


  </div> <!-- body -->




<?php include_once("footer.php"); ?>
