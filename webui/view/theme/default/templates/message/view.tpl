<div id="restorebox" class="alert alert-general">
<?php if(Registry::get('auditor_user') == 1 && count($rcpt) > 0) { ?>
<?php foreach($rcpt as $r) { ?>
      <input type="checkbox" class="restorebox" id="rcpt_<?php print $r; ?>" name="rcpt_<?php print $r; ?>" value="1" /> <?php print $r; ?><br />
<?php } ?>
<br />
<input type="button" id="restore_button" name="restore_button" value="<?php print $text_restore; ?>" class="btn btn-primary" onclick="Piler.restore_message_for_recipients(<?php print $id; ?>, '<?php print $text_restored; ?>', '<?php print $text_select_recipients; ?>');" />
<input type="button" value="<?php print $text_cancel; ?>" class="btn btn-inverse" onclick="$('#restorebox').hide();" />
<?php } ?>

</div>

<div class="messageheader">

    <p>
    <?php /*if(SMARTHOST || ENABLE_IMAP_AUTH == 1) { ?>
       <a class="messagelink" href="#" onclick="Piler.restore_message(<?php print $id; ?>);"><i class="icon-reply"></i>&nbsp;<?php print $text_restore_to_mailbox; ?></a> |
    <?php } */?>
       <a class="messagelink" href="#" onclick="Piler.view_headers(<?php print $id; ?>);"><i class="icon-info"></i>&nbsp;<?php print $text_view_headers; ?></a> |
    <?php if($spam == 1) { ?>
       | <a class="messagelink" href="#" onclick="Piler.release_message(<?php print $id; ?>, <?php print $spam; ?>);"><i class="icon-envelope"></i>&nbsp;<?php print $text_not_spam; ?></a>
    <?php } else { ?>
       | <a class="messagelink" href="#" onclick="Piler.release_message(<?php print $id; ?>, <?php print $spam; ?>);"><i class="icon-exclamation-sign"></i>&nbsp;<?php print $text_spam; ?></a>
    <?php } ?>
       | <a class="messagelink" href="#" onclick="Piler.remove_message(<?php print $id; ?>);"><i class="icon-remove"></i>&nbsp;<?php print $text_remove; ?></a>
    </p>
</div>

<div id="messageblock">

<div class="messageheader">
    <strong><?php if($message['subject'] == "" || $message['subject'] == "Subject:") { print "&lt;" . $text_no_subject . "&gt;"; } else { print $message['subject']; } ?></strong><br />
    <strong><?php print $message['from']; ?></strong><br />
    <strong><?php print $message['to']; ?></strong><br />
    <strong><?php print $message['date']; ?></strong><br />
</div>
<div class="messagecontents">
<?php print $message['message']; ?>
</div>

<?php foreach($images as $img) { ?>
   <p><img src="<?php print SITE_URL; ?>/tmp/<?php print $img['name']; ?>" alt="" /></p>
<?php } ?>


</div>

