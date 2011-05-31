
<form action="index.php?route=quarantine/quarantine" name="aaa0" method="get" onsubmit="fix_search(); return false;">

   <input type="hidden" name="user" value="<?php print $username; ?>" />
   <table border="0">
     <tr>
      <td><?php print $text_from; ?>:</td><td><input type="text" name="from" value="<?php print $from; ?>" /></td>
      <td><?php print $text_subject; ?>:</td><td><input type="text" name="subj" value="<?php print $subj; ?>" /></td>
      <td>Ham/spam:
         <select name="hamspam" onChange="javascript:ham_or_spam();">
<?php if(SPAM_ONLY_QUARANTINE == 0) { ?>
            <option value=""<?php if($hamspam == ""){ ?> selected="selected"<?php } ?>>All</option>
            <option value="HAM"<?php if($hamspam == "HAM"){ ?> selected="selected"<?php } ?>>HAM</option>
<?php } ?>
            <option value="SPAM"<?php if($hamspam == "SPAM"){ ?> selected="selected"<?php } ?>>SPAM</option>
         </select>
      </td>
      <td colspan="2"><input type="submit" value="<?php print $text_submit; ?>" /></td>
     </tr>
<?php if($searchterms) { ?>
     <tr>
      <td colspan="3">
         <select name="searchterm" onChange="fix_search_terms('<?php print $username; ?>');">
            <option value="<?php if(SPAM_ONLY_QUARANTINE == 1) { ?>index.php?route=quarantine/quarantine&amp;hamspam=SPAM<?php } ?>"><?php print $text_search_terms; ?></option>
            <option value="index.php?route=quarantine/quarantine<?php if(SPAM_ONLY_QUARANTINE == 1) { ?>&amp;hamspam=SPAM<?php } ?>"><?php print $text_empty_search_criteria; ?></option>
<?php foreach($searchterms as $term) { ?>
            <option value="index.php?route=quarantine/quarantine&<?php print $term['term']; ?>&user=<?php print $username; ?>"<?php if(strncmp($_SERVER['QUERY_STRING'], "route=quarantine/quarantine&" . $term['term'] . "&user=" . $username, strlen("route=quarantine/quarantine&" . $term['term'] . "&user=" . $username)) == 0){ ?> selected="selected"<?php } ?>><?php $q1 = 0; parse_str($term['term'], $x1); if($x1['subj']){ print "subj: " . $x1['subj']; $q1++; } if($x1['from']){ if($q1) print ", "; print "from: " . $x1['from']; $q1++; } if($x1['hamspam']){  if($q1) print ", "; print $x1['hamspam']; } ?></option>
<?php } ?>
         </select>
      </td>
     </tr>
<?php } ?>

   </table>
</form>


<?php if($n > 0){ ?>

<p><?php print $text_number_of_messages_in_quarantine; ?>: <?php print $n; ?> (<?php print $total_size; ?> bytes)</p>

<div id="pagenav">
   <?php if($page > 0){ ?><a href="index.php?route=quarantine/quarantine&amp;page=0&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &laquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php if($page > 0){ ?><a href="index.php?route=quarantine/quarantine&amp;page=<?php print $prev_page; ?>&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam;?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &lsaquo; <?php if($page > 0){ ?></a><?php } ?>
    <?php print $messages[0]['i']; ?> - <?php print $messages[count($messages)-1]['i']; ?>
   <?php if($n > $messages[count($messages)-1]['i']){ ?><a href="index.php?route=quarantine/quarantine&amp;page=<?php print $next_page; ?>&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam;?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &rsaquo; <?php if($n >= $page_len*($page+1) && $n > $page_len){ ?></a><?php } ?>
   <?php if($n > $messages[count($messages)-1]['i']){ ?><a href="index.php?route=quarantine/quarantine&amp;page=<?php print $total_pages; ?>&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &raquo; <?php if($page < $total_pages){ ?></a><?php } ?>
</div>

<div id="quarantine">

<form action="index.php?route=quarantine/remove" name="aaa1" method="post">
   <input type="hidden" name="topurge" value="1" />
   <input type="hidden" name="user" value="<?php print $username; ?>" />

<p>
   <table border="0" id="thetable">
   <tr align="middle">
      <th>&nbsp;</th>
      <th><?php print $text_date; ?> <a href="index.php?route=quarantine/quarantine&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=ts&amp;order=0"><img src="view/theme/<?php print THEME; ?>/images/arrowup.gif" border="0"></a> <a href="index.php?route=quarantine/quarantine&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=ts&amp;order=1"><img src="view/theme/<?php print THEME; ?>/images/arrowdown.gif" border="0"></a></th>
      <th><?php print $text_username; ?></th>
      <th><?php print $text_from; ?> <a href="index.php?route=quarantine/quarantine&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=from&amp;order=0"><img src="view/theme/<?php print THEME; ?>/images/arrowup.gif" border="0"></a> <a href="index.php?route=quarantine/quarantine&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=from&amp;order=1"><img src="view/theme/<?php print THEME; ?>/images/arrowdown.gif" border="0"></a></th>
      <th><?php print $text_subject; ?> <a href="index.php?route=quarantine/quarantine&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=subj&amp;order=0"><img src="view/theme/<?php print THEME; ?>/images/arrowup.gif" border="0"></a> <a href="index.php?route=quarantine/quarantine&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=subj&amp;order=1"><img src="view/theme/<?php print THEME; ?>/images/arrowdown.gif" border="0"></a></th>
      <th><?php print $text_size; ?> <a href="index.php?route=quarantine/quarantine&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=size&amp;order=0"><img src="view/theme/<?php print THEME; ?>/images/arrowup.gif" border="0"></a> <a href="index.php?route=quarantine/quarantine&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>&amp;sort=size&amp;order=1"><img src="view/theme/<?php print THEME; ?>/images/arrowdown.gif" border="0"></a></th>
      <th>&nbsp;</th>
      <th>&nbsp;</th>
      <th>&nbsp;</th>
   </tr>
   <tr>
      <td>&nbsp;</td>
      <td>&nbsp;</td>
      <td>&nbsp;</td>
      <td width="35%"><img src="<?php print WEBUI_DIRECTORY; ?>/view/theme/<?php print THEME; ?>/images/line.png" alt="xxx" /></td>
      <td width="40%"><img src="<?php print WEBUI_DIRECTORY; ?>/view/theme/<?php print THEME; ?>/images/line.png" alt="xxx" /></td>
      <td>&nbsp;</td>
      <td>&nbsp;</td>
      <td>&nbsp;</td>
      <td>&nbsp;</td>
   </tr>

<?php foreach ($messages as $message) { ?>

   <tr valign="top" id="tr_<?php print $message['id'] . "+" . $message['username']; ?>">
      <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><a href="index.php?route=quarantine/message&amp;id=<?php print $message['id']; ?>&amp;user=<?php print $username; ?>&amp;page=<?php print $page; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>"><?php print $message['i']; ?>.</a></td>
      <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><?php print $message['date']; ?></td>
      <td align="center"<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><a href="index.php?route=quarantine/quarantine&user=<?php print $message['username']; ?>"><?php print $message['username']; ?></a></td>
      <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><?php if($message['from'] != $message['shortfrom']) { ?><span onmouseover="Tip('<?php print preg_replace("/&/", "&amp;", $message['from']); ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $message['shortfrom']; ?></span><?php } else { print $message['from']; } ?></td>
      <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><a href="index.php?route=quarantine/message&amp;id=<?php print $message['id']; ?>&amp;user=<?php print $username; ?>&amp;page=<?php print $page; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>"><?php if($message['subject'] != $message['shortsubject']) { ?><span onmouseover="Tip('<?php print preg_replace("/&/", "&amp;", $message['subject']); ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $message['shortsubject']; ?></span><?php } else { print $message['subject']; } ?></a></td>
      <td align="right"><?php print $message['size']; ?></td>
      <td>&nbsp;</td>
      <td class="<?php if($message['id'][0] == 's'){ ?>spam<?php } else { ?>ham<?php } ?>">&nbsp;</td>
      <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><input type="checkbox" name="<?php print $message['id'] . "+" . $message['username']; ?>" onChange="script:toggle_row_highlight('tr_<?php print $message['id'] . "+" . $message['username']; ?>');" /></td>
   </tr>

<?php } ?>

   </table>
</p>


<p>
   <input type="reset" value="<?php print $text_cancel; ?>"  onClick="highlight_table(false)" />
   <input type="button" value="<?php print $text_select_all; ?>" onClick="mark_all(true)" />
</p>

<p>
   <select name="massaction">
      <option value="-"><?php print $text_select_action; ?></option>
      <option value="purge"><?php print $text_purge_selected_messages; ?></option>
      <option value="deliver"><?php print $text_deliver_selected_messages; ?></option>
      <option value="train"><?php if(SPAM_ONLY_QUARANTINE == 1) { print $text_train_selected_messages_as_ham; } else { print $text_train_selected_messages; } ?></option>
      <option value="train+deliver"><?php if(SPAM_ONLY_QUARANTINE == 1) { print $text_deliver_and_train_selected_messages_as_ham; } else { print $text_deliver_and_train_selected_messages; } ?></option>
      <option value="purgeeverything"><?php print $text_purge_all_messages_from_quarantine; ?></option>
   </select>

   <input type="button" name="MassAction" value="Ok" onClick="fix_mass_action('<?php print $subj; ?>', '<?php print $from; ?>', '<?php print $hamspam; ?>', '<?php print $username; ?>', <?php print $page; ?>);" />
</p>

</form>

</div>


<p>&nbsp;</p>

<div id="pagenav">
   <?php if($page > 0){ ?><a href="index.php?route=quarantine/quarantine&amp;page=0&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>" class="navlink"><?php } ?> &laquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php if($page > 0){ ?><a href="index.php?route=quarantine/quarantine&amp;page=<?php print $prev_page; ?>&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam;?>" class="navlink"><?php } ?> &lsaquo; <?php if($page > 0){ ?></a><?php } ?>
    <?php print $messages[0]['i']; ?> - <?php print $messages[count($messages)-1]['i']; ?>
   <?php if($n > $messages[count($messages)-1]['i']){ ?><a href="index.php?route=quarantine/quarantine&amp;page=<?php print $next_page; ?>&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam;?>" class="navlink"><?php } ?> &rsaquo; <?php if($n >= $page_len*($page+1) && $n > $page_len){ ?></a><?php } ?>
   <?php if($n > $messages[count($messages)-1]['i']){ ?><a href="index.php?route=quarantine/quarantine&amp;page=<?php print $total_pages; ?>&amp;user=<?php print $username; ?>&amp;from=<?php print $from; ?>&amp;subj=<?php print $subj; ?>&hamspam=<?php print $hamspam; ?>" class="navlink"><?php } ?> &raquo; <?php if($page < $total_pages){ ?></a><?php } ?>
</div>



<?php } else { ?>
<?php print $text_no_message_in_the_quarantine; ?>
<?php } ?>

