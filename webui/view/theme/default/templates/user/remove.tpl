
<p>

<?php if($confirmed){ ?>

<?php print $x; ?>. <a href="index.php?route=user/list"><?php print $text_back; ?></a>

<?php } else { ?>
<a href="index.php?route=user/remove&amp;uid=<?php print $uid; ?>&amp;email=<?php print $email; ?>&amp;confirmed=1"><?php print $text_remove_this_user; ?>: <?php print $email; ?></a>
<?php } ?>

</p>

