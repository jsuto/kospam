
<p>
   <a href="index.php?route=quarantine/remove&amp;id=<?php print $id; ?>&amp;user=<?php print $username; ?>&amp;page=<?php print $page; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>"><?php print $text_remove; ?></a>
   <?php if($id[0] == 's'){ ?><a href="index.php?route=quarantine/deliver&amp;id=<?php print $id; ?>&amp;user=<?php print $username; ?>&amp;page=<?php print $page; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>"><?php print $text_deliver; ?></a><?php } ?>
   <a href="index.php?route=quarantine/train&amp;id=<?php print $id; ?>&amp;user=<?php print $username; ?>&amp;nodeliver=1&amp;page=<?php print $page; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>"><?php print $text_train; ?></a>
   <?php if($id[0] == 's') { ?><a href="index.php?route=quarantine/train&amp;id=<?php print $id; ?>&amp;user=<?php print $username; ?>&amp;page=<?php print $page; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>"><?php print $text_train_and_deliver; ?></a><?php } ?>
<?php if($raw == 1){ ?>
   <a href="index.php?route=quarantine/message&amp;id=<?php print $id; ?>&amp;user=<?php print $username; ?>&amp;page=<?php print $page; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>"><?php print $text_view_formatted_email; ?></a>
<?php } else { ?>
   <a href="index.php?route=quarantine/message&amp;id=<?php print $id; ?>&amp;raw=1&amp;user=<?php print $username; ?>&amp;page=<?php print $page; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>"><?php print $text_view_raw_email; ?></a>
<?php } ?>
</p>


<?php print $message; ?>

