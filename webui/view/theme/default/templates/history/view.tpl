<?php if($entry){ ?>

<table border="1">
   <tr><td>clapf id: </td><td><?php if($entry['username']) { ?><a href="index.php?route=quarantine/message&id=<?php if($entry['result'] == "SPAM") { ?>s.<?php } else { ?>h.<?php } ?><?php print $entry['clapf_id']; ?>&user=<?php print $entry['username']; ?>"><?php } ?><?php print $entry['clapf_id']; ?><?php if($entry['username']) { ?></a><?php } ?></td></tr>
   <tr><td><?php print $text_date; ?>: </td><td><?php print $entry['timedate']; ?></td></tr>
   <tr><td><?php print $text_clienthost; ?>: </td><td><?php print $entry['client']; ?></td></tr>
   <tr><td><?php print $text_from; ?>: </td><td><?php print $entry['from']; ?></td></tr>
   <tr><td><?php print $text_to; ?>: </td><td><?php print $entry['to']; ?></td></tr>

   <tr><td>Queue id: </td><td><?php print $entry['queue_id1']; ?></td></tr>
   <tr><td>Message id: </td><td><?php print $entry['message_id']; ?></td></tr>
   <tr><td><?php print $text_size; ?>: </td><td><?php print $entry['size']; ?></td></tr>

   <tr><td><?php print $text_content_filter; ?>: </td><td><?php print $entry['content_filter']; ?></td></tr>
   <tr><td><?php print $text_delay; ?>: </td><td><?php print $entry['delay']; ?></td></tr>
   <tr><td><?php print $text_result; ?>: </td><td><?php print $entry['result']; ?> (<?php printf("%.4f", $entry['spaminess']); ?>)</td></tr>

   <tr><td>Relay: </td><td><?php print $entry['relay']; ?></td></tr>
   <tr><td><?php print $text_delivered; ?>: </td><td><?php print $entry['status']; ?></td></tr>
   </tr>
</table>

<?php } else { ?>
<?php print $text_no_records; ?>
<?php } ?>
