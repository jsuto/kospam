
<?php if(isset($_SESSION['username'])){
   print $text_you_are; ?>: <?php print $_SESSION['username'];
} else { ?>
<form name="login" action="index.php?route=login/login" method="post">
   <table border="0">
      <tr><td><?php print $text_username; ?>:</td><td><input type="text" name="username" /></td></tr>
      <tr><td><?php print $text_password; ?>:</td><td><input type="password" name="password" /></td></tr>
      <tr><td>&nbsp;</td><td><input type="submit" value="<?php print $text_submit; ?>" /> <input type="reset" value="<?php print $text_cancel; ?>" /></td></tr>
   </table>
</form>

<?php } ?>

