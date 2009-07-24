<p>

<?php if(isset($x)){ ?>
<?php print $x; ?>. <a href="index.php?route=user/blacklist"><?php print $text_back; ?></a>
<?php } else { ?>

<form name="settings" action="index.php?route=user/blacklist" method="post">
   <input type="hidden" name="modify" value="1">
   <table border="0">
      <tr valign="top"><td><textarea name="blacklist" cols="30" rows="10"><?php print $blacklist; ?></textarea></td></tr>
      <tr valign="top"><td><input type="submit" value="<?php print $text_submit; ?>"> <input type="reset" value="<?php print $text_cancel; ?>"></td></tr>
   </table>
</form>

<?php } ?>

</p>

