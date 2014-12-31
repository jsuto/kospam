<div>

<?php if($confirmed){ ?>

<div class="alert alert-success"><?php print $x; ?>.</div>
<p><a href="index.php?route=policy/list"><i class="icon-circle-arrow-left"></i>&nbsp;<?php print $text_back; ?></a></p>

<?php } else { ?>

<p><a href="index.php?route=policy/edit&amp;id=<?php print $id; ?>"><i class="icon-circle-arrow-left"></i>&nbsp;<?php print $text_back; ?></a> | <a href="index.php?route=policy/remove&amp;id=<?php print $id; ?>&amp;name=<?php print $name; ?>&amp;confirmed=1"><i class="icon-remove-sign"></i>&nbsp;<?php print $text_remove; ?>: <?php print $name; ?></a></p>

<?php } ?>

</div>

