<?

include_once("config.php");
include_once("header.inc");

$modify = 0;
$whitelist = "";

if(isset($_POST['modify'])) $modify = $_POST['modify'];
if(isset($_POST['whitelist'])) $whitelist = $_POST['whitelist'];

$username = $_SERVER['REMOTE_USER'];
if($username == "") nice_error($err_not_authenticated);

$conn = webui_connect() or nice_error($err_connect_db);

?>


  <h3><? print $WHITELIST_SETTINGS; ?></h3>

  <div id="body">

<p>

<?

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




<? include_once("footer.inc"); ?>
