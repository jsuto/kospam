
<p/>

<h4><?php print $text_bulk_update_selected_uids; ?></h4>

<?php if(isset($policies)) { ?>

<form action="index.php?route=user/massedit" name="massedit" class="form-inline" method="post">
   <input type="hidden" name="edit" value="1" />
<?php foreach ($uids as $uid) { ?>
   <input type="hidden" name="aa_<?php print $uid; ?>" value="1" />
<?php } ?>

      <div class="row">
         <div class="healthcell"><?php print $text_uids; ?>: <?php print $uidlist; ?></div>
      </div>

      <div class="row">
       <div class="healthcell"><?php print $text_domain; ?>:</div><div class="healthcell">
         <select name="domain" class="span4">
<?php foreach ($domains as $domain) { ?>
            <option value="<?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?>"><?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?></option>
<?php } ?>
         </select>
       </div>
      </div>

      <div class="row">
       <div class="healthcell"><?php print $text_policy_group; ?>:</div><div class="healthcell">
         <select name="policy_group" class="span4">
            <option value="0"><?php print DEFAULT_POLICY; ?></option>
<?php foreach ($policies as $policy) { ?>
            <option value="<?php print $policy['policy_group']; ?>"><?php print $policy['name']; ?></option>
<?php } ?>
         </select>
       </div>
      </div>
      <div class="row"><div class="healthcell"><?php print $text_group_id; ?>:</div><div class="healthcell"><input type="text" name="gid" value="" class="span4" /></div></div>
      <div class="row"><div class="healthcell"><?php print $text_whitelist; ?>:</div><div class="healthcell"><textarea name="whitelist" class="span4"></textarea></div></div>
      <div class="row"><div class="healthcell"><?php print $text_blacklist; ?>:</div><div class="healthcell"><textarea name="blacklist" class="span4"></textarea></div></div>
      <div class="row"><div class="healthcell">&nbsp;</div><div class="healthcell"><input type="submit" value="<?php print $text_update_selected_uids; ?>" class="btn btn-primary" /></div></div>
</form>



<!--h4><?php print $text_remove_selected_uids; ?></h4>

<p><?php print $text_uids; ?>: <?php print $uidlist; ?></p>


<form action="index.php?route=user/massedit" name="massremove" method="post" class="form-inline">
   <input type="hidden" name="remove" value="1" />
<?php foreach ($uids as $uid) { ?>
   <input type="hidden" name="aa_<?php print $uid; ?>" value="1" />
<?php } ?>
   <input type="submit" value="<?php print $text_remove_selected_uids; ?>" class="btn btn-secondary" />
</form-->


<?php } else if(isset($x)){ print $x; ?>. <a href="index.php?route=user/list"><?php print $text_back; ?></a>
<?php } ?>


