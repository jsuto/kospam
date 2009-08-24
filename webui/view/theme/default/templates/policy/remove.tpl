
<p>

<?php if($confirmed){ ?>

<?php print $x; ?>. <a href="index.php?route=policy/policy"><?php print $text_back; ?></a>

<?php } else { ?>
<a href="index.php?route=policy/remove&amp;policy_group=<?php print $policy_group; ?>&amp;confirmed=1"><?php print $text_remove_policy; ?>: <?php print $policy_name; ?></a>
<?php } ?>

</p>

