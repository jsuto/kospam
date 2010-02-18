
<?php /*foreach ($users as $user){ ?>
   <a href="index.php?route=quarantine/quarantine&user=<?php print $user['name']; ?>"><?php print $user['name']; ?></a><br/>
<?php } */ ?>

<form name="searchuser" action="index.php?route=quarantine/quarantine" onSubmit="fix_form(); return false;">
   <?php print $text_username; ?>: <input type="text" name="user" value="" /> <input type="submit" value="<?php print $text_submit; ?>" />
</form>
