
<p>
<?php if(Registry::get('admin_user') == 1 && $_SESSION['train_global'] && (int)@$_GET['globaltrain'] == 1) { print $text_global_training_happened; } ?>

<?php print $text_trained; ?>: <?php print $n; ?> <?php print $text_messages; ?>. <a href="index.php?route=quarantine/quarantine&amp;user=<?php print $username; ?>&amp;page=<?php print $page; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>"><?php print $text_back; ?></a>
</p>
