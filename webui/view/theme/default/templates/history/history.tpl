<?php if($page == 0){ ?>


<form class="form-inline" style="padding-bottom: 20px;" onsubmit="return false;">

<?php print $text_date; ?>: <input type="text" name="date1" id="date1" class="input-small" value="<?php print $date1; ?>" placeholder="YYYY-MM-DD" /> - <input type="text" name="date2" id="date2" class="input-small" value="<?php print $date2; ?>" placeholder="YYYY-MM-DD" />

<?php print $text_sender; ?>: <input type="text" class="input-small" name="sender_domain" id="sender_domain" value="<?php print $sender_domain; ?>" onchange="Clapf.filter_history();" />
<?php print $text_recipient; ?>: <input type="text" class="input-small" name="rcpt_domain" id="rcpt_domain" value="<?php print $rcpt_domain; ?>" onchange="Clapf.filter_history();" />
<?php print $text_subject; ?>: <input type="text" class="input-small" name="subject" id="subject" value="<?php print $subject; ?>" onchange="Clapf.filter_history();" />

Ham/spam: <select name="hamspam" id="hamspam" onchange="Clapf.filter_history();" class="input-small">
   <option value="" <?php if($hamspam == ""){ ?>selected="selected" <?php } ?>/>All
   <option value="HAM" <?php if($hamspam == "HAM"){ ?>selected="selected" <?php } ?>/>HAM
   <option value="SPAM" <?php if($hamspam == "SPAM"){ ?>selected="selected" <?php } ?>/>SPAM
</select>

<button class="btn btn-ok" onclick="Clapf.filter_history();"><?php print $text_set; ?></button>

</form>

<?php } ?>

<span id="A1"><?php print $text_loading; ?> . . .</span>
