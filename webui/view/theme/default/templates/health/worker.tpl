<table border="0" cellpadding="10">
<tr valign="top">
   <td>

<p><?php print $text_refresh_period; ?>: <?php print HEALTH_REFRESH; ?> sec</p>

<p><strong><?php print $text_uptime; ?>: </strong><?php print $uptime; ?></strong></p>

<p><strong><?php print $text_processed_emails_in; ?>:</strong> <?php print $processed_emails[0]; ?>/<?php print $processed_emails[1]; ?>/<?php print $processed_emails[2]; ?></p>

<p><strong><?php print $text_quarantined_emails; ?>: </strong><?php print $number_of_quarantined_messages; ?></p>


<p><strong><?php print $text_smtp_status; ?>: </strong></p>

<p>
<table border="1">
<?php foreach($health as $h) {
   $status = 'ERROR';
   if(preg_match("/^220/", $h[1]) || preg_match("/^action=DUNNO/", $h[1])) { $status = 'OK'; }

?>
   <tr>
      <td><?php print $h[3]; ?></td>
      <td class="<?php if($status == 'OK') { ?>HAM<?php } else { ?>SPAM<?php } ?>"><span onmouseover="Tip('<?php print preg_replace("/\'/", "\'", $h[1]); ?>, <?php print $h[2]; ?>', BALLOON, true, ABOVE,
true)" onmouseout="UnTip()"><?php print $status; ?></span></td>
      <!--td class="<?php if($status == 'OK') { ?>ham<?php } else { ?>spam<?php } ?>">&nbsp;</td-->
   </tr>
<?php } ?>
</table>
</p>


<?php if(ENABLE_LDAP_IMPORT_FEATURE == 1) { ?><p><strong><?php print $text_ad_sync_status; ?>:</strong></p> <p><span class="health-<?php if($totalusers >= LDAP_IMPORT_MINIMUM_NUMBER_OF_USERS_TO_HEALTH_OK && $total_emails_in_database >= LDAP_IMPORT_MINIMUM_NUMBER_OF_USERS_TO_HEALTH_OK) { ?>ok<?php } else { ?>alert<?php } ?>"><?php print $adsyncinfo; ?></span></p><?php } ?>

<p><strong><?php print $text_cpu_usage; ?>:</strong> <span class="health-<?php if($cpuinfo < HEALTH_RATIO) { ?>ok<?php } else { ?>alert<?php } ?>"><?php print $cpuinfo; ?>%</span>, <strong><?php print $text_cpu_load; ?>:</strong> <span class="health-<?php if($cpuinfo < HEALTH_RATIO) { ?>ok<?php } else { ?>alert<?php } ?>"><?php print $cpuload; ?></span></p>

<p><strong><?php print $text_memory_usage; ?>: </strong> <span class="health-<?php if($meminfo < HEALTH_RATIO) { ?>ok<?php } else { ?>alert<?php } ?>"><?php print $meminfo; ?>%</span> / <?php print $totalmem; ?> MB, <strong><?php print $text_swap_usage; ?></strong>: <span class="health-<?php if($swapinfo < HEALTH_RATIO) { ?>ok<?php } else { ?>alert<?php } ?>"><?php print $swapinfo; ?>%</span> / <?php print $totalswap; ?> MB</p>

<p><strong><?php print $text_disk_usage; ?>: </strong> <?php foreach($shortdiskinfo as $partition) { ?><span class="health-<?php if($partition['utilization'] < HEALTH_RATIO) { ?>ok<?php } else { ?>alert<?php } ?>"><?php print $partition['partition']; ?> <?php print $partition['utilization']; ?>%</span> <?php } ?></p>

<p><strong><?php print $text_counters; ?>:</strong></p>

<p>
<table border="1">
<?php while(list($k, $v) = each($counters)) { ?>
   <tr><td><?php $a = preg_replace("/^_c\:/", "", $k); if(isset($$a)) { print $$a; } else { print $k; } ?></td><td><?php print $v; ?></td></tr>
<?php } ?>
   <?php if($counters[$prefix . 'rcvd'] > 0) { ?><tr><td>spam / <?php print $text_total_ratio; ?></td><td><?php print sprintf("%.2f", 100*$counters[$prefix . 'spam'] / $counters[$prefix . 'rcvd']); ?> %</td></tr><?php } ?>
   <?php if($counters[$prefix . 'rcvd'] > 0) { ?><tr><td>virus / <?php print $text_total_ratio; ?></td><td><?php print sprintf("%.2f", 100*$counters[$prefix . 'virus'] / $counters[$prefix . 'rcvd']); ?> %</td></tr><?php } ?>

</table>
</p>

<form action="index.php?route=health/worker" method="post">
   <input type="hidden" name="resetcounters" value="1" />
   <input type="submit" name="submit" value="<?php print $text_reset_counters; ?>" />
</form>

   </td>
   <td>

<p><strong><?php print $text_queue_status; ?>: </strong></p>

<?php foreach ($queues as $queue) { ?>

<p><table border="0">

<?php if(isset($queue['desc'])) { ?>

    <td valign="top">
       <strong><?php print $queue['desc']; ?></strong><br />
      <table border="0">
<?php 
   $i = 0;
   while(list($k, $v) = each($queue['lines'])) {
      $i++;
      print "<tr class='queue'>";
      $v = preg_replace("/^\*\<\/td\>/", "", $v); 
      if($i == 1) { print "<td>&nbsp;</td>"; }
      print "$v</td></tr>\n"; 
      if($i == count($queue['lines'])-1) { break; }
   }
?>
</table>
 
    </td>

<?php } ?>

</table></p>

<?php } ?>


   </td>
</tr>
</table>

