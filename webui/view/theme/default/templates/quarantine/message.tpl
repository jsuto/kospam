
<p>
   <button onclick="Clapf.remove_message('<?php print $username; ?>', '<?php print $id; ?>');" class="btn btn-danger"><?php print $text_remove; ?></button>
   <?php if($id[0] == 's' || $id[0] == 'v'){ ?><button onclick="Clapf.deliver_message('<?php print $username; ?>', '<?php print $id; ?>');" class="btn btn-primary"><?php if(TRAIN_DELIVERED_SPAM == 1) { print $text_train_and_deliver_as_ham; } else { print $text_deliver; } ?></button><?php } ?>
</p>



<strong><?php if($message['subject'] == "" || $message['subject'] == "Subject:") { print "&lt;" . $text_no_subject . "&gt;"; } else { print $message['subject']; } ?></strong><br />
<strong><?php print $message['from']; ?></strong><br />
<strong><?php print $message['to']; ?></strong><br />
<strong><?php print $message['date']; ?></strong><br />

<?php print $message['message']; ?><br />

