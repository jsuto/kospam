<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

if($username == "") show_auth_popup();

include_once("header.php");

$modify = 0;
$blacklist = "";

if(isset($_POST['modify'])) $modify = $_POST['modify'];
if(isset($_POST['blacklist'])) $blacklist = $_POST['blacklist'];

$conn = webui_connect() or nice_error($err_connect_db);

?>


  <h3><?php print $BLACKLIST_SETTINGS; ?></h3>

  <div id="body">

<p>

<?php

if($modify == 1){
   set_blacklist($blacklist, $username);
   nice_screen("$err_successfully_modified. <a href=\"blacklist.php\">$BACK.</a>");
}

else {
   $blacklist = get_blacklist_by_name($username);

   print "<form name=\"settings\" action=\"blacklist.php\" method=\"post\">\n";
   print "<input type=\"hidden\" name=\"modify\" value=\"1\">\n";
   print "<table border=\"0\">\n";

   print "<tr valign=\"top\"><td><textarea name=\"blacklist\" cols=\"30\" rows=\"10\">$blacklist</textarea></td></tr>\n";

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
