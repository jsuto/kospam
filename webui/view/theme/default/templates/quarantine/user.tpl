
<form name="searchuser" action="index.php?route=quarantine/quarantine&hamspam=SPAM" onSubmit="fix_form(); return false;">
   <?php print $text_username; ?>: <input type="text" name="user" value="" /> <input type="submit" value="<?php print $text_submit; ?>" />
</form>
