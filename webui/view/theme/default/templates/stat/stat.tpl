
<p>&nbsp;</p>

<p>

<?php if($timespan == 0){ ?>
<strong><?php print $text_daily_report; ?></strong> <a href="index.php?route=stat/stat&timespan=1"><?php print $text_monthly_report; ?></a>
<?php } else { ?>
<a href="index.php?route=stat/stat&timespan=0"><?php print $text_daily_report; ?></a> <strong><?php print $text_monthly_report; ?></strong>
<?php } ?>

</p>


<p>
   <img src="index.php?route=stat/graph&timespan=<?php print $timespan; ?>&uid=<?php print $uid; ?>" />
</p>

