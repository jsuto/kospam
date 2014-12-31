
<div id="sspinner" class="alert alert-info lead"><i class="icon-spinner icon-spin icon-2x pull-left"></i><?php print $text_working; ?></div>
<div id="messagelistcontainer" class="boxlistcontent">
    <?php if($n > 0) { ?>
    <table id="results" class="table table-striped table-condensed">
      <thead>
        <tr>
          <th id="id-header">&nbsp;</th>
          <th id="date-header">
             <?php print $text_date; ?>
          </th>
          <th id="from-header">
             <?php print $text_from; ?>
          </th>
          <th id="to-header">
             <?php print $text_to; ?>
          </th>
          <th id="subject-header">
             <?php print $text_subject; ?>
          </th>
          <th id="size-header">
             <?php print $text_size; ?>
          </th>
          <th id="spam-header">Spam</th>
          <th id="relay-header"><?php print $text_relay; ?></th>
          <th id="status-header"><?php print $text_status; ?></th>
       </tr>
      </thead>
      <tbody>
    <?php $i=0; foreach ($messages as $message) { ?>
            
         <tr id="e_<?php print $message['id']; ?>" class="resultrow new">
            <td id="c2_r<?php print $i; ?>" class="resultcell id"><?php print ($page*$page_len) + $i + 1; ?>.</td>
            <td id="c3_r<?php print $i; ?>" class="resultcell date"><?php print $message['date']; ?></td>
            <td id="c4_r<?php print $i; ?>" class="resultcell from"><?php if($message['from'] != $message['shortfrom']) { ?><span title="<?php print $message['from']; ?>"><?php print $message['shortfrom']; ?></span><?php } else { print $message['from']; } ?></td>
            <td id="c5_r<?php print $i; ?>" class="resultcell to"><?php if($message['to'] != $message['shortto']) { ?><span title="<?php print $message['to']; ?>"><?php print $message['shortto']; ?>&nbsp;<i class=" muted icon-group"></i></span><?php } else { print $message['to']; } ?></td>
            <td id="c4_r<?php print $i; ?>" class="resultcell subject"><?php if($message['subject'] != $message['shortsubject']) { ?><span title="<?php print $message['subject']; ?>"><?php print $message['shortsubject']; ?></span><?php } else { print $message['subject']; } ?></td>
            <td id="c7_r<?php print $i; ?>" class="resultcell size"><?php print $message['size']; ?></td>
            <td id="c8_r<?php print $i; ?>" class="resultcell end"><?php if($message['spam'] == 1) { ?><i class="spam icon-warning-sign icon-large" title="<?php print $text_spam_flag; ?>"></i><?php } else { ?>&nbsp;<?php } ?></td>
            <td id="c9_r<?php print $i; ?>" class="resultcell end"><?php print $message['relay']; ?></td>
            <td id="c9_r<?php print $i; ?>" class="resultcell end"><?php print $message['status']; ?></td>
         </tr>

    <?php $i++; } ?>
      </tbody>
      
    </table>

    <?php } else if($n == 0) { ?>
                <div class="alert alert-block alert-error lead"><i class="icon-exclamation-sign icon-2x pull-left"></i> <?php print $text_empty_search_result; ?></div>
    <?php } ?>

</div>

<div id="messagelistfooter" class="boxfooter">
    <div class="row-fluid">
       <div id="pagingrow" class="span4">
            <div id="pagingbox">
    <?php if($n > 0){ ?>
            &nbsp;
            <?php if($page > 0) { ?><a href="#" class="navlink" onclick="Piler.navigation(0);"><i class="icon-double-angle-left icon-large"></i></a><?php } else { ?><span class="navlink"><i class="icon-double-angle-left icon-large muted"></i></span><?php } ?>
            &nbsp;
            <?php if($page > 0) { ?><a href="#" class="navlink" onclick="Piler.navigation(<?php print $prev_page; ?>);"><i class="icon-angle-left icon-large"></i></a><?php } else { ?><span class="navlink"><i class="icon-angle-left icon-large muted"></i></span><?php } ?>
            &nbsp;
            <?php print $hits_from; ?>-<?php print $hits_to; ?>, <?php print $text_total; ?>: <?php print $n; ?><?php if($total_found > $n) { ?> (<?php print $total_found; ?>)<?php } ?>
            &nbsp;
            <?php if($next_page <= $total_pages){ ?><a href="#" class="navlink" onclick="Piler.navigation(<?php print $next_page; ?>);"><i class="icon-angle-right icon-large"></i></a> <?php } else { ?><span class="navlink"><i class="icon-angle-right icon-large muted"></i></span><?php } ?>
            &nbsp;
            <?php if($page < $total_pages) { ?><a href="#" class="navlink" onclick="Piler.navigation(<?php print $total_pages; ?>);"><i class="icon-double-angle-right icon-large"></i></a><?php } else { ?> <span class="navlink"><i class="icon-double-angle-right icon-large muted"></i></span><?php } ?>
            &nbsp;

    <?php } else { print $text_none_found; } ?>
            </div>
        </div>
        <div id="functionrow" class="span8">
        </div>
    </div>
</div>
