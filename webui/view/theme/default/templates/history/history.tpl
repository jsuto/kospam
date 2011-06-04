<?php if($page == 0){ ?>

<form action="" method="get">

<input type="hidden" name="hamspam" value="" />

<p><?php print $text_date; ?>: <input type="text" name="date1" id="date1" length="11" size="11" value="<?php print $date1; ?>" /> - <input type="text" name="date2" id="date2" size="11" value="<?php print $date2; ?>" />,

<?php print $text_sender; ?>: <input type="text" name="sender_domain" value="<?php print $sender_domain; ?>" onChange="javascript:filterhistory(); loadHistory('<?php print HISTORY_WORKER_URL; ?>');" />, <?php print $text_recipient; ?>: <input type="text" name="rcpt_domain" value="<?php print $rcpt_domain; ?>" onChange="javascript:filterhistory(); loadHistory('<?php print HISTORY_WORKER_URL; ?>');" />,
<?php print $text_subject; ?>: <input type="text" name="subject" value="<?php print $subject; ?>" onChange="javascript:filterhistory(); loadHistory('<?php print HISTORY_WORKER_URL; ?>');" />

<input type="button" name="set_button" value="<?php print $text_set; ?>" onClick="javascript:filterhistory(); loadHistory('<?php print HISTORY_WORKER_URL; ?>');" />

</p>

</form>

<?php } ?>

<span id="A1"><?php print $text_loading; ?> . . .</span>
