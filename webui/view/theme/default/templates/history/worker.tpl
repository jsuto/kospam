<?php if(count($entries) > 0){ ?>

<table border="1">
   <tr align="center">
   <th><?php print $text_time; ?></th>
   <th><?php print $text_from; ?></th>
   <th><?php print $text_to; ?></th>
   <th><?php print $text_size; ?></th>
   <th><?php print $text_subject; ?></th>
   <th>SPAM</th>
   <th><?php print $text_status; ?></th>
   </tr>

<?php foreach ($entries as $entry) { ?>
   <tr>
      <td><?php print $entry['timedate']; ?></td>
      <td><span onmouseover="Tip('<?php if($entry['client']) { ?>client host: <?php print $entry['client']; ?><br/><?php } ?>sender:  <?php print $entry['from']; ?><br/><?php if($entry['queue_id']) { ?>queue id: <?php print $entry['queue_id']; ?><br/><?php } ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $entry['shortfrom']; ?></span></td>
      <td><?php if($entry['to'] != $entry['shortto']) { ?><span onmouseover="Tip('Recipient: <?php print $entry['to']; ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php } print $entry['shortto']; ?><?php if($entry['to'] != $entry['shortto']) { ?></span><?php } ?></td>
      <td align="right"><?php print $entry['size']; ?></td>
      <td align="left"><span onmouseover="Tip('<?php print $entry['subject']; ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $entry['shortsubject']; ?></span></td>
      <td align="center" class="<?php print $entry['result']; ?>"><?php print $entry['result']; ?></td>
      <td align="center"><span onmouseover="Tip('<?php print $entry['delivery']; ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $entry['shortdelivery']; ?></span></td>
   </tr>
<?php } ?>

</table>

<p><?php print $text_total_query_time; ?>: <?php print sprintf("%.4f", $tot_time); ?> sec</p>

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

