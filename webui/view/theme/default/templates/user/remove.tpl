
<p>

<?php if($confirmed){ ?>

<?php print $x; ?>. <a href="index.php?route=user/list"><?php print $text_back; ?></a>

<?php } else { ?>
<a href="index.php?route=user/remove&uid=<?php print $uid; ?>&email=<?php print $email; ?>&confirmed=1"><?php print $text_remove_this_user; ?>: <?php print $email; ?></a>
<?php } ?>

</p>

