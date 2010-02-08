<?php if($page == 0){ ?>

<form action="" method="get">

<p>
Ham/spam: <select name="hamspam" onChange="javascript:ham_or_spam();">
   <option value="" <?php if($hamspam == ""){ ?>selected="selected" <?php } ?>/>All
   <option value="HAM" <?php if($hamspam == "HAM"){ ?>selected="selected" <?php } ?>/>HAM
   <option value="SPAM" <?php if($hamspam == "SPAM"){ ?>selected="selected" <?php } ?>/>SPAM
</select>

Sender domain: <input type="text" name="sender_domain" value="<?php print $sender_domain; ?>" />, Recipient domain: <input type="text" name="rcpt_domain" value="<?php print $rcpt_domain; ?>" />

<input type="button" name="set_button" value="<?php print $text_set; ?>" onClick="javascript:filterhistory();" />
</p>

</form>

<?php } ?>

<span id="A1"><?php print $text_loading; ?> . . .</span>
