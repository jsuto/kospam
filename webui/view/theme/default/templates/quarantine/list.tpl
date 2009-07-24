
<form action="index.php?route=quarantine/quarantine" name="aaa0" method="get" onsubmit="fix_search(); return false;">
   <input type="hidden" name="user" value="<?php print $username; ?>">
   <table border="0">
      <tr><td><?php print $text_from; ?>:</td><td><input type="text" name="from" value="<?php print $from; ?>"></td></tr>
      <tr><td><?php print $text_subject; ?>:</td><td><input type="text" name="subj" value="<?php print $subj; ?>"></td></tr>
      <tr colspan="2"><td><input type="submit" value="<?php print $text_submit; ?>"></td></tr>
   </table>
</form>

<?php if($n_spam > 0){ ?>

<p><?php print $text_number_of_spam_messages_in_quarantine; ?>: <?php print $n_spam; ?> (<?php print $spam_total_size; ?> bytes)</p>

<form action="index.php?route=quarantine/remove" name="aaa1" method="post">
   <input type="hidden" name="topurge" value="1">
   <input type="hidden" name="user" value="<?php print $username; ?>">

<p>
   <table border="0">
   <tr align="middle">
      <th>&nbsp;</th>
      <th><?php print $text_date; ?></th>
      <th><?php print $text_from; ?></th>
      <th><?php print $text_subject; ?></th>
      <th>&nbsp;</th>
   </tr>
   <tr>
      <td>&nbsp;</td>
      <td>&nbsp;</td>
      <td><font color="#ffffff"><?php print str_repeat("x", 3+MAX_CGI_FROM_SUBJ_LEN); ?></font></td>
      <td><font color="#ffffff"><?php print str_repeat("x", 3+MAX_CGI_FROM_SUBJ_LEN); ?></font></td>
      <td>&nbsp;</td>
   </tr>

<?php foreach ($messages as $message) { ?>

   <tr valign="top">
      <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><a href="index.php?route=quarantine/message&id=<?php print $message['id']; ?>&user=<?php print $username; ?>"><?php print $message['i']; ?>.</a></td><td><?php print $message['date']; ?></td><td><?php print $message['from']; ?></td>
      <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><a href="index.php?route=quarantine/message&id=<?php print $message['id']; ?>&user=<?php print $username; ?>"><?php print $message['subject']; ?></a></td>
      <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><input type="checkbox" name="<?php print substr($message['id'], 2, strlen($message['id'])); ?>"></td>
   </tr>

<?php } ?>

   </table>
</p>


<p>
   <input type="reset" value="<?php print $text_cancel; ?>">
   <input type="button" value="<?php print $text_select_all; ?>" onClick="mark_all(true)">
</p>

<p>
   <input type="submit" value="<?php print $text_purge_selected_messages; ?>">
   <input type="button" value="<?php print $text_deliver_selected_messages; ?>" onClick='document.forms.aaa1.action="index.php?route=quarantine/massdeliver&user=<?php print $username; ?>"; document.forms.aaa1.submit();'>
   <input type="button" value="<?php print $text_deliver_and_train_selected_messages; ?>" onClick='document.forms.aaa1.action="index.php?route=quarantine/masstrain&user=<?php print $username; ?>"; document.forms.aaa1.submit();'>
</p>

</form>


<form action="index.php?route=quarantine/remove" name="purgeallfromqueue" method="post">
<p>
   <input type="hidden" name="purgeallfromqueue" value="1">
   <input type="hidden" name="user" value="<?php print $username; ?>">
   <input type="submit" value="<?php print $text_purge_all_messages_from_quarantine; ?>">
</p>
</form>

<?php if($n_spam > $page_len){ ?>
<p>
<?php if($page > 0){ ?>
   <a href="index.php?route=quarantine/quarantine&page=0&user=<?php print $username; ?>&from=<?php print $from; ?>&subj=<?php print $subj; ?>"><?php print $text_first; ?></a>
   <a href="index.php?route=quarantine/quarantine&page=<?php print $prev_page; ?>&user=<?php print $username; ?>&from=<?php print $from; ?>&subj=<?php print $subj; ?>"><?php print $text_previous; ?></a>
<?php } ?>

<?php if($n_spam >= $page_len*($page+1) && $n_spam > $page_len){ ?>
   <a href="index.php?route=quarantine/quarantine&page=<?php print $next_page; ?>&user=<?php print $username; ?>&from=<?php print $from; ?>&subj=<?php print $subj; ?>"><?php print $text_next; ?></a>
<?php } ?>

<?php if($page < $total_pages){ ?>
   <a href="index.php?route=quarantine/quarantine&page=<?php print $total_pages; ?>&user=<?php print $username; ?>&from=<?php print $from; ?>&subj=<?php print $subj; ?>"><?php print $text_last; ?></a>
<?php } ?>
</p>
<?php } ?>



<?php } else { ?>
<?php print $text_no_spam_message_in_the_quarantine_yet; ?>
<?php } ?>

