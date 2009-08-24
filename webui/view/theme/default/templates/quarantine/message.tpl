
<p>
   <a href="index.php?route=quarantine/remove&amp;id=<?php print $id; ?>&amp;user=<?php print $username; ?>"><?php print $text_remove; ?></a>
   <a href="index.php?route=quarantine/deliver&amp;id=<?php print $id; ?>&amp;user=<?php print $username; ?>"><?php print $text_deliver; ?></a>
   <a href="index.php?route=quarantine/train&amp;id=<?php print $id; ?>&amp;user=<?php print $username; ?>"><?php print $text_train_and_deliver; ?></a>
<?php if($raw == 1){ ?>
   <a href="index.php?route=quarantine/message&amp;id=<?php print $id; ?>&amp;user=<?php print $username; ?>"><?php print $text_view_formatted_email; ?></a>
<?php } else { ?>
   <a href="index.php?route=quarantine/message&amp;id=<?php print $id; ?>&amp;raw=1&amp;user=<?php print $username; ?>"><?php print $text_view_raw_email; ?></a>
<?php } ?>
</p>


<?php print $message; ?>

