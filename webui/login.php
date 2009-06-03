<?php

include_once("config.php");

session_start();

$login = "";
$pwd = "";

if(isset($_POST['login'])) $login = $_POST['login'];
if(isset($_POST['pwd'])) $pwd = $_POST['pwd'];

if($login && $pwd){
   $conn = webui_connect();

   $ok = check_user_auth($login, $pwd);

   webui_close($conn);

   if($ok == 1) header("Location: $base_url" . "/q.php");
}


include_once("header.php");

?>


  <h3><?php print $LOGIN; ?></h3>

  <div id="body">

   <table border="0" cellspacing="0" cellpadding="0">
    <tr valign="top">
      <td>

<p>

<?php

if(!isset($_SESSION['username'])){
   print "<form name=\"login\" action=\"$base_url/login.php\" method=\"post\">\n";
   print "<table border=\"0\" cellpadding=\"0\" cellspacing=\"5\">\n";
   print "<tr><td>$USERNAME:</td><td><input type=\"text\" name=\"login\"></td></tr>\n";
   print "<tr><td>$PASSWORD:</td><td><input type=\"password\" name=\"pwd\"></td></tr>\n";
   print "<tr><td>&nbsp;</td><td><input type=\"submit\" value=\"$SUBMIT\"> <input type=\"reset\" value=\"$CANCEL\"></td></tr>\n";
   print "</table>\n";
   print "</form>\n";
}
else {
   print "$YOU_ARE: " . $_SESSION['username'];
}

?>


</p>

      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<?php include_once("footer.php"); ?>

