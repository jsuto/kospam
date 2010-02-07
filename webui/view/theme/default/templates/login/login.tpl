
<?php if(!isset($_SESSION['username'])){ ?>
<form name="login" action="index.php?route=login/login" method="post">
   <table border="0">
      <tr><td><?php print $text_email; ?>:</td><td><input type="text" name="username" /></td></tr>
      <tr><td><?php print $text_password; ?>:</td><td><input type="password" name="password" /></td></tr>
      <tr><td>&nbsp;</td><td><input type="submit" value="<?php print $text_submit; ?>" /> <input type="reset" value="<?php print $text_cancel; ?>" /></td></tr>
   </table>
</form>

<?php if(isset($x)){ ?><p class="loginfailed"><?php print $x; ?></p><?php } ?>

<?php } ?>

