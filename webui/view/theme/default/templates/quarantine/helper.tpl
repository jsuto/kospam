


<div id="messagelistcontainer" class="boxlistcontent">

   <div id="results">


   <input type="hidden" name="topurge" value="1" />
   <input type="hidden" name="user" value="<?php print $username; ?>" />

<?php if($n > 0){ ?>


   <table class="table table-condensed table-striped table-hover" id="thetable" name="thetable">
      <thead>

         <tr align="middle">
            <th>&nbsp;</th>
            <th>
               <strong><?php print $text_date; ?></strong>
               <a xid="date" xorder="0" onclick="Clapf.changeOrder(this);"><i class="icon-chevron-up"></i></a>
               <a xid="date" xorder="1" onclick="Clapf.changeOrder(this);"><i class="icon-chevron-down"></i></a>
            </th>
            <th><?php print $text_username; ?></th>
            <th>
               <strong><?php print $text_from; ?></strong>
               <a xid="from" xorder="0" onclick="Clapf.changeOrder(this);"><i class="icon-chevron-up"></i></a>
               <a xid="from" xorder="1" onclick="Clapf.changeOrder(this);"><i class="icon-chevron-down"></i></a>
            </th>
            <th>
               <strong><?php print $text_subject; ?><strong>
               <a xid="subject" xorder="0" onclick="Clapf.changeOrder(this);"><i class="icon-chevron-up"></i></a>
               <a xid="subject" xorder="1" onclick="Clapf.changeOrder(this);"><i class="icon-chevron-down"></i></a>
            </td>
            <th>
               <strong><?php print $text_size; ?></strong>
               <a xid="size" xorder="0" onclick="Clapf.changeOrder(this);"><i class="icon-chevron-up"></i></a>
               <a xid="size" xorder="1" onclick="Clapf.changeOrder(this);"><i class="icon-chevron-down"></i></a>
            </th>
            <th>&nbsp;</th>
            <th>&nbsp;</th>
            <th><input type="checkbox" name="bulkcheck" id="bulkcheck" onclick="Clapf.toggle_bulk_check();" /></th>
         </tr>

      </thead>
      <tbody>

<?php $i=0; foreach ($messages as $message) { ?>

         <tr id="tr_<?php print $message['id'] . "+" . $message['username']; ?>" onclick="Clapf.toggle_row_highlight('tr_<?php print $message['id'] . "+" . $message['username']; ?>');">
            <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><?php print $message['i']; ?>.</td>
            <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><?php print $message['date']; ?></td>
            <td onclick="Clapf.stop_propagation(event);" align="center"<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><a href="#" onclick="Clapf.change_quarantine_user('<?php print $message['username']; ?>', '<?php if(SPAM_ONLY_QUARANTINE == 1) { ?>SPAM<?php } ?>'); "><?php print $message['username']; ?></a></td>
            <td<?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><?php if($message['from'] != $message['shortfrom']) { ?><span title="<?php print $message['from']; ?>"><?php print $message['shortfrom']; ?></span><?php } else { print $message['from']; } ?></td>
            <td onclick="Clapf.stop_propagation(event);" <?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><a href="#" onclick="Clapf.view_message_by_pos('<?php print $i; ?>'); return false;"><?php if($message['subject'] != $message['shortsubject']) { print $message['shortsubject']; } else { print $message['subject']; } ?></a></td>


            <td align="right"><?php print $message['size']; ?></td>
            <td>&nbsp;</td>
            <td class="<?php if($message['id'][0] == 's'){ ?>error<?php } else if($message['id'][0] == 'v') { ?>virus<?php } else { ?>success<?php } ?>">&nbsp;</td>
            <td onclick="Clapf.stop_propagation(event);" <?php if(($message['i'] % 2) == 0){ ?> class="odd"<?php } ?>><input type="checkbox" id="<?php print $message['id'] . "+" . $message['username']; ?>" name="<?php print $message['id'] . "+" . preg_replace("/\./", "*", $message['username']); ?>" onChange="script:Clapf.toggle_row_highlight('tr_<?php print $message['id'] . "+" . $message['username']; ?>');" /></td>
         </tr>

<?php $i++; } ?>
      </tbody>

   </table>

</div>

<script>
//Clapf.fill_current_messages_array();
</script>



<?php } else { ?>
<?php print $text_no_message_in_the_quarantine; ?>
<?php } ?>


   </div>

</div>

     <div class="boxfooter">
        <form class="form-inline sleek" name="tagging">
<?php if($n >= $page_len){ ?>
   <span class="piler-right-margin">
         <?php if($page > 0) { ?><a href="#" onclick="Clapf.navigation(0);">&lt;&lt;</a> &nbsp; <?php } else { ?><span class="navlink">&lt;&lt; &nbsp; </span><?php } ?>
         <?php if($page > 0) { ?><a href="#" onclick="Clapf.navigation(<?php print $prev_page; ?>);"> &lt; </a> <?php } else { ?><span class="navlink"> &lt; </span><?php } ?>

         <?php print $hits_from; ?>-<?php print $hits_to; ?>, <?php print $text_total; ?>: <?php print $n; ?>

         <?php if($next_page <= $total_pages){ ?><a href="#" onclick="Clapf.navigation(<?php print $next_page; ?>);">&gt; </a> <?php } else { ?><span class="navlink">&gt; </span><?php } ?>
         <?php if($page < $total_pages) { ?> &nbsp; <a href="#" onclick="Clapf.navigation(<?php print $total_pages; ?>);"> &gt;&gt; </a><?php } else { ?> <span class="navlink"> &nbsp; &gt;&gt;</span><?php } ?>
   </span>
<?php } else { ?>&nbsp;<?php } ?>

           <button class="btn piler-right-margin" onclick="Clapf.remove_messages(); return false;"><?php print $text_purge_selected_messages; ?></button>
           <button class="btn piler-right-margin" onclick="Clapf.remove_all_messages(); return false;"><?php print $text_purge_all_messages_from_quarantine; ?></button>
           <button class="btn piler-right-margin" onclick="Clapf.deliver_messages(); return false;"><?php if(TRAIN_DELIVERED_SPAM == 1) { print $text_deliver_and_train_selected_messages; } else { print $text_deliver_selected_messages; } ?></button>

        </form>
     </div>


