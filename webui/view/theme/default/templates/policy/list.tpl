<h4><?php print $text_existing_policies; ?></h4>

<div class="listarea bottom">

<?php if(isset($policies)){ ?>

   <div id="ss1">
      <div class="domainrow">
         <div class="domaincell"><?php print $text_policy; ?></div>
         <div class="domaincell">&nbsp;</div>
      </div>

<?php foreach($policies as $policy) { ?>
      <div class="domainrow">
         <div class="domaincell"><a href="index.php?route=policy/view&policy_group=<?php print $policy['policy_group']; ?>"><?php print $policy['name']; ?></a></div>
         <div class="domaincell"><a href="index.php?route=policy/remove&amp;confirmed=1&amp;policy_group=<?php print $policy['policy_group']; ?>" onclick="if(confirm('<?php print $text_remove_policy; ?>: ' + '\'<?php print $policy['name']; ?>\'')) return true; return false;"><?php print $text_remove_this_policy; ?></a></div>
      </div>
<?php } ?>

   </div>

<?php } else { ?>
<?php print $text_not_found; ?>
<?php } ?>

</div>



<p><a href="index.php?route=policy/add"><?php print $text_add_policy; ?></a></p>

