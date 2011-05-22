<?php if($page == 0){ ?>

<form action="" method="get">

<input type="hidden" name="hamspam" value="" />

<p><?php print $text_date; ?>: <input type="text" name="date1" id="date1" length="11" size="11" value="" /> - <input type="text" name="date2" id="date2" size="11" value="" />,

<?php print $text_sender; ?>*: <input type="text" name="sender_domain" value="<?php print $sender_domain; ?>" />, <?php print $text_recipient; ?>*: <input type="text" name="rcpt_domain" value="<?php print $rcpt_domain; ?>" />

<input type="button" name="set_button" value="<?php print $text_set; ?>" onClick="javascript:filterhistory(); loadHistory('<?php print HISTORY_WORKER_URL; ?>');" />
</p>

</form>

<p>*: <?php print $text_exact_domain_name_or_email_address; ?></p>

<?php } ?>

<span id="A1"><?php print $text_loading; ?> . . .</span>
