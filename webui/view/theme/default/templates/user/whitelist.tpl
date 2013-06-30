
<?php if(isset($x)){ ?>
<?php print $x; ?>. <a href="index.php?route=user/whitelist"><?php print $text_back; ?></a>
<?php } else { ?>

<form name="settings" action="index.php?route=user/whitelist" method="post">
   <input type="hidden" name="modify" value="1" />
   <div class="row"><div class="healthcell"><textarea name="whitelist" class="span6 bw"><?php print $whitelist; ?></textarea></div></div>
   <div class="row"><div class="healthcell"><input type="submit" class="btn btn-primary" value="<?php print $text_submit; ?>" /> <input type="reset" class="btn" value="<?php print $text_cancel; ?>" /></div></div>
</form>

<?php } ?>


