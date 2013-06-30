
<p>
<?php if(Registry::get('admin_user') == 1 && $_SESSION['train_global'] && $globaltrain == 1) { print $text_global_training_happened; } ?>

<span class="text-success"><strong><?php print $message; ?> <?php if($n > -1) { print $n; } ?></strong></span>
</p>

