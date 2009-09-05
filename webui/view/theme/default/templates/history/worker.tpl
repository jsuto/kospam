<table border="1">
   <tr align="center">
   <th><?php print $text_time; ?></th>
   <th><?php print $text_from; ?>*</th>
   <th><?php print $text_to; ?></th>
   <th><?php print $text_size; ?></th>
   <th><?php print $text_result; ?>*</th>
   <th><?php print $text_status; ?>*</th>
   </tr>

<?php foreach ($entries as $entry) { ?>
   <tr>
      <td><?php print $entry['timedate']; ?></td>
      <td><span onmouseover="Tip('sender:  <?php print $entry['from']; ?><br/>queue id: <?php print $entry['queue_id1']; ?><br/>message id: <?php print $entry['message_id']; ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $entry['shortfrom']; ?></span></td>
      <td><?php print $entry['to']; ?></td>
      <td align="right"><?php print $entry['size']; ?></td>
      <td align="center" class="<?php print $entry['result']; ?>"><span onmouseover="Tip('content filter: <?php print $entry['content_filter']; ?><br/>relay: <?php print $entry['relay']; ?><br/>clapf id: <?php print $entry['clapf_id']; ?><br/>spaminess: <?php printf("%.2f", $entry['spaminess']); ?><br/>delay: <?php print $entry['delay']; ?><br/>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $entry['result']; ?></span></td>
      <td align="center"><span onmouseover="Tip('<?php print $entry['status_balloon']; ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $entry['status']; ?></span></td>
   </tr>
<?php } ?>

</table>

