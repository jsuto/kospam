<?php if(count($entries) > 0){ ?>

<div class="row">
   <div class="domaincell center"><?php print $text_time; ?></div>
   <div class="domaincell center"><?php print $text_from; ?></div>
   <div class="domaincell center"><?php print $text_to; ?></div>
   <div class="domaincell center"><?php print $text_size; ?></div>
   <div class="domaincell center"><?php print $text_subject; ?></div>
   <div class="domaincell center">SPAM</div>
   <div class="domaincell center"><?php print $text_status; ?></div>
</div>

<?php foreach ($entries as $entry) { ?>
   <div class="row">
      <div class="domaincell no-bold"><?php print $entry['timedate']; ?></div>
      <div class="domaincell no-bold"><span title="<?php if($entry['client']) { ?>client host: <?php print $entry['client']; ?>, <?php } ?>sender:  <?php print $entry['from']; ?>, <?php if($entry['queue_id']) { ?>queue id: <?php print $entry['queue_id']; ?><?php } ?>"><?php print $entry['shortfrom']; ?></span></div>
      <div class="domaincell no-bold"><span title="<?php if($entry['to'] != $entry['shortto']) { print $entry['to']; } ?>"><?php print $entry['shortto']; ?></span></div>
      <div class="domaincell no-bold"><?php print $entry['size']; ?></div>
      <div class="domaincell no-bold"><span title="<?php if($entry['subject'] != $entry['shortsubject']) { print $entry['subject']; } ?>"><?php print $entry['shortsubject']; ?></span></div>
      <div class="domaincell no-bold <?php if($entry['result'] == 'HAM') { ?>text-success<?php } else { ?>text-error<?php } ?>"><?php print $entry['result']; ?></div>
      <div class="domaincell no-bold"><span title="<?php print $entry['delivery']; ?>"><?php print $entry['shortdelivery']; ?></span></div>
   </div>
<?php } ?>


<h5 class="left"><?php print $text_total_query_time; ?>: <?php print sprintf("%.4f", $tot_time); ?> sec</h5>

        <form class="form-inline sleek left" name="tagging">
<?php if($total > $page_len){ ?>
   <span class="piler-right-margin">
         <?php if($page > 0) { ?><a href="#" onclick="Clapf.history_navigation(0);">&lt;&lt;</a> &nbsp; <?php } else { ?><span class="navlink">&lt;&lt; &nbsp; </span><?php } ?>
         <?php if($page > 0) { ?><a href="#" onclick="Clapf.history_navigation(<?php print $prev_page; ?>);"> &lt; </a> <?php } else { ?><span class="navlink"> &lt; </span><?php } ?>

         <?php print $hits_from; ?>-<?php print $hits_to; ?>, <?php print $text_total; ?>: <?php print $total; ?>

         <?php if($next_page <= $total_pages){ ?><a href="#" onclick="Clapf.history_navigation(<?php print $next_page; ?>);">&gt; </a> <?php } else { ?><span class="navlink">&gt; </span><?php } ?>
         <?php if($page < $total_pages) { ?> &nbsp; <a href="#" onclick="Clapf.history_navigation(<?php print $total_pages; ?>);"> &gt;&gt; </a><?php } else { ?> <span class="navlink"> &nbsp; &gt;&gt;</span><?php } ?>
   </span>
<?php } else { ?>&nbsp;<?php } ?>

        </form>



<?php } else { ?>
<p><?php print $text_no_records; ?></p>
<?php } ?>

