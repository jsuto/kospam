<?php if(count($entries) > 0){ ?>

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
      <td><a href="index.php?route=history/view&id=<?php print $entry['clapf_id']; ?>&to=<?php print $entry['to']; ?>"><?php print $entry['timedate']; ?></a></td>
      <td><span onmouseover="Tip('client host: <?php print $entry['client']; ?><br/>sender:  <?php print $entry['from']; ?><br/>queue id: <?php print $entry['queue_id1']; ?><br/>message id: <?php print $entry['message_id']; ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $entry['shortfrom']; ?></span></td>
      <td><?php if($entry['to'] != $entry['shortto']) { ?><span onmouseover="Tip('Recipient: <?php print $entry['to']; ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php } print $entry['shortto']; ?><?php if($entry['to'] != $entry['shortto']) { ?></span><?php } ?></td>
      <td align="right"><?php print $entry['size']; ?></td>
      <td align="center" class="<?php print $entry['result']; ?>"><span onmouseover="Tip('content filter: <?php print $entry['content_filter']; ?><br/>relay: <?php print $entry['relay']; ?><br/>clapf id: <?php print $entry['clapf_id']; ?><br/>spaminess: <?php printf("%.2f", $entry['spaminess']); ?><br/>delay: <?php print $entry['delay']; ?><br/>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $entry['result']; ?></span></td>
      <td align="center"><span onmouseover="Tip('<?php print $entry['status_balloon']; ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $entry['status']; ?></span></td>
   </tr>
<?php } ?>

</table>


<?php if($total > $page_len){ ?>
<p>
<?php if($page > 0){ ?>
   <a href="index.php?route=history/history&amp;page=0&amp;search=<?php print $search; ?>"><?php print $text_first; ?></a>
   <a href="index.php?route=history/<?php if($prev_page > 0) { ?>history<?php } else { ?>history<?php } ?>&amp;page=<?php print $prev_page; ?>&amp;search=<?php print $search; ?>"><?php print $text_previous; ?></a>
<?php } ?>

<?php if($total >= $page_len*($page+1) && $total > $page_len){ ?>
   <a href="index.php?route=history/history&amp;page=<?php print $next_page; ?>&amp;search=<?php print $search; ?>"><?php print $text_next; ?></a>
<?php } ?>

<?php if($page < $total_pages){ ?>
   <a href="index.php?route=history/history&amp;page=<?php print $total_pages; ?>&amp;search=<?php print $search; ?>"><?php print $text_last; ?></a>
<?php } ?>
</p>
<?php } ?>

<?php } else { ?>
<p><?php print $text_no_records; ?></p>
<?php } ?>

