
<?php if(isset($x)){ ?>
<?php print $x; ?>. <a href="index.php?route=user/whitelist"><?php print $text_back; ?></a>
<?php } else { ?>

<form name="settings" action="index.php?route=user/whitelist" method="post">
   <input type="hidden" name="modify" value="1" />
   <table border="0">
      <tr valign="top"><td><textarea name="whitelist" cols="30" rows="10"><?php print $whitelist; ?></textarea></td></tr>
      <tr valign="top"><td><input type="submit" value="<?php print $text_submit; ?>" /> <input type="reset" value="<?php print $text_cancel; ?>" /></td></tr>
   </table>
</form>

<?php } ?>


