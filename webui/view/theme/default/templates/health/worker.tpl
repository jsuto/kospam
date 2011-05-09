
<p>&nbsp;</p>

<p><?php print $text_refresh_period; ?>: <?php print HEALTH_REFRESH; ?> sec</p>

<p><strong><?php print $text_uptime; ?>: </strong><?php print $uptime; ?></p>

<p><strong><?php print $text_processed_emails_in; ?>:</strong> <?php print $processed_emails[0]; ?>/<?php print $processed_emails[1]; ?>/<?php print $processed_emails[2]; ?></p>

<p><strong><?php print $text_quarantined_emails; ?>: </strong><?php print "aaaa"; ?></p>


<p><strong><?php print $text_smtp_status; ?>: </strong></p>

<p>
<table border="1">
<?php foreach($health as $h) { ?>
   <tr>
      <td><?php print $h[0]; ?></td>
      <td class="<?php if(!preg_match("/^220/", $h[1])) { ?>SPAM<?php } ?>"><?php print $h[1]; ?></td>
      <td><?php print $h[2]; ?></td>
      <td class="<?php if(preg_match("/^220/", $h[1])) { ?>ham<?php } else { ?>spam<?php } ?>">&nbsp;</td>
   </tr>
<?php } ?>
</table>
</p>


<p><strong><?php print $text_latest_emails; ?></strong></p>

<p>
<table border="1">
   <tr align="center">
   <th><?php print $text_time; ?></th>
   <th><?php print $text_from; ?></th>
   <th><?php print $text_recipient; ?></th>
   <th><?php print $text_subject; ?></th>
   </tr>

<?php foreach ($emails as $email) {

   if(isset($email['queue_id'])) { ?>

   <tr>
      <td><a href="index.php?route=history/view&id=<?php print $email['queue_id']; ?>&to=<?php print $email['to']; ?>"><?php print date("Y.m.d. H:i:s", $email['ts']); ?></a></td>
      <td><?php print $email['from']; ?></td>
      <td><?php print $email['to']; ?></td>
      <td><?php if(strlen($email['subject'])) { print substr($email['subject'], 0, 2*FROM_LENGTH_TO_SHOW) . "..."; } else { print $email['subject']; } ?></td>

   </tr>

<?php } } ?>

</table>
</p>


<p><strong><?php print $text_queue_status; ?>: </strong></p>

<?php foreach ($queues as $queue) { ?>

<?php if(isset($queue['desc'])) { ?>

<p>
<table border="1">
   <tr><td colspan="12"><strong><?php print $queue['desc']; ?></strong> <?php print $queue['date']; ?></td></tr>
   <?php
    $i = 0;
    while(list($k, $v) = each($queue['lines'])) {
      $i++;
      $v = preg_replace("/^\ {1,}/", "", $v);
       ?>
       <tr align="right"><?php if($i==1){ ?><td>&nbsp;</td><?php } ?><td><?php print preg_replace("/\ {1,}/", "</td><td>", $v); ?> </td></tr>

   <?php
       if($i == count($queue['lines'])-1) { break; }
   } ?>
</table>
</p>

<?php } ?>

<?php } ?>


<p><strong><?php print $text_memory_usage; ?>: </strong></p>
<pre>
<?php print $meminfo; ?>
</pre>

<p><strong><?php print $text_disk_usage; ?>: </strong></p>
<pre>
<?php print $diskinfo; ?>
</pre>


