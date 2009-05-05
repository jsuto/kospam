<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

include_once("header.php");

if($username == "") show_auth_popup();

if(isset($_POST['password']) && isset($_POST['password2'])){
   if($_POST['password'] != $_POST['password2']) nice_error($err_password_mismatch);
   if(strlen($_POST['password']) < $min_password_len) nice_error($err_too_short_password);

   if(change_password() == 1) nice_screen($err_password_changed);
   else nice_error($err_failed_to_change_password);
}


if(isset($_GET['pagelen'])){
   Header("Set-Cookie: pagelen=" . $_GET['pagelen'] . "; path=/");
}

?>


  <h3><?php print $HOME; ?></h3>

  <div id="body">

   <table border="0" cellspacing="0" cellpadding="0">
    <tr valign="top">
      <td>

<p>

<?php print "$YOU_ARE: $username"; ?>

</p>

<p>
<form name="pwdchange" action="<?php print $_SERVER['PHP_SELF']; ?>" method="post">
<table border="0" cellpadding="5" cellspacing="5">
<tr><td><?php print $PASSWORD; ?>:</td><td><input type="password" name="password"></td></tr>
<tr><td><?php print $PASSWORD_AGAIN; ?>:</td><td><input type="password" name="password2"></td></tr>
<tr><td>&nbsp;</td><td><input type="submit" value="<?php print $SUBMIT; ?>"> <input type="reset" value="<?php print $CANCEL; ?>"></td></tr>
</table>
</form>
</p>

<p>
<form action="index.php" method="get">
<?php print $PAGE_LENGTH; ?>: <select name="pagelen">
   <option value="10">10
   <option value="20">20
   <option value="30">30
   <option value="50">50
</select>
<input type="submit" value="<?php print $SET; ?>" />
</form>

</p>

      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<?php include_once("footer.php"); ?>
