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

   if($ok == 1) header("Location: $base_url");
}


include_once("header.php");

?>


  <h3><?php print $LOGIN; ?></h3>

  <div id="body">

   <table border="0" cellspacing="0" cellpadding="0">
    <tr valign="top">
      <td>

<p>
<form name="login" action="<?php print "$base_url/login.php"; ?>" method="post">
<table border="0" cellpadding="0" cellspacing="5">
<tr><td><?php print $USERNAME; ?>:</td><td><input type="text" name="login"></td></tr>
<tr><td><?php print $PASSWORD; ?>:</td><td><input type="password" name="pwd"></td></tr>
<tr><td>&nbsp;</td><td><input type="submit" value="<?php print $SUBMIT; ?>"> <input type="reset" value="<?php print $CANCEL; ?>"></td></tr>
</table>
</form>
</p>

      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<?php include_once("footer.php"); ?>

