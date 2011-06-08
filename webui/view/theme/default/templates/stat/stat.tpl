
<p>&nbsp;</p>

<p>

<?php if($timespan == "daily"){ ?>
<strong><?php print $text_daily_report; ?></strong> <a href="index.php?route=stat/stat&amp;timespan=monthly<?php if(isset($uid)) { ?>&amp;uid=<?php print $uid; } ?>"><?php print $text_monthly_report; ?></a>
<?php } else { ?>
<a href="index.php?route=stat/stat&amp;timespan=daily<?php if(isset($uid)) { ?>&amp;uid=<?php print $uid; } ?>"><?php print $text_daily_report; ?></a> <strong><?php print $text_monthly_report; ?></strong>
<?php } ?>

</p>


<p><img src="index.php?route=stat/graph&amp;timespan=<?php print $timespan; ?>&amp;uid=<?php print $uid; ?>" border="1" /> <img src="index.php?route=stat/chart&amp;timespan=<?php print $timespan; ?>" border="1" /></p>

<?php if($admin_user == 1 || $readonly_admin == 1) { ?>
<p><img src="index.php?route=stat/topdomains&amp;timespan=<?php print $timespan; ?>&amp;what=SPAM" border="1" /> <img src="index.php?route=stat/topdomains&amp;timespan=<?php print $timespan; ?>&amp;what=HAM" border="1" /></p>
<?php } ?>

