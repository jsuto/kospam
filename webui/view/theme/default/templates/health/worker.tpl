
<p>&nbsp;</p>

<p><?php print $text_last_update; ?>: <?php print date("Y.m.d. H:i:s"); ?>. <?php print $text_refresh_period; ?>: <?php print HEALTH_REFRESH; ?> sec</p>

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


<p><strong><?php print $text_latest_emails; ?></strong></p>

<p>
<table border="1">
   <tr align="center">
   <th><?php print $text_time; ?></th>
   <th><?php print $text_recipient; ?></th>
   <th>clapf id</th>
   <th>Relay</th>
   <th><?php print $text_result; ?></th>
   </tr>

<?php foreach ($emails as $email) {

   if(isset($email['queue_id'])) { ?>

   <tr>
      <td><a href="index.php?route=history/view&id=<?php print $email['queue_id']; ?>&to=<?php print $email['rcpt']; ?>"><?php print date("Y.m.d. H:i:s", $email['ts']); ?></a></td>
      <td><?php print $email['rcpt']; ?></td>
      <td><?php print $email['queue_id']; ?></td>
      <td><?php print $email['relay']; ?></td>
      <td><?php print $email['result']; ?></td>
   </tr>

<?php } } ?>

</table>
</p>


