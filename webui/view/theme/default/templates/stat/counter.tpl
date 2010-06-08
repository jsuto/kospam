
<p>&nbsp;</p>

<p>
<table border="1">
<?php while(list($k, $v) = each($counters)) { ?>
   <tr><td><?php print $k; ?></td><td><?php print $v; ?></td></tr>
<?php } ?>
</table>
</p>

<p>
<form action="index.php?route=stat/counter" method="post">
   <input type="hidden" name="reset" value="1" />
   <input type="submit" name="submit" value="<?php print $text_reset_counters; ?>" />
</form>
</p>
