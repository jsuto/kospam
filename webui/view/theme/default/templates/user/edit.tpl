

<?php if(isset($user)) {

   $userbasedn = preg_replace("/cn=([\w]+),/", "", $user['dn']); ?>

<form action="index.php?route=user/edit" name="adduser" method="post" class="form-inline" autocomplete="off">
   <input type="hidden" name="uid" value="<?php print $uid; ?>" />

      <div class="row">
         <div class="healthcell"><?php print $text_email_addresses; ?>:</div>
         <div class="healthcell"><textarea name="email" class="span4"><?php print $emails; ?></textarea></div>
      </div>

      <div class="row"><div class="healthcell"><?php print $text_username; ?>:</div><div class="healthcell"><input type="text" name="username" value="<?php print $user['username']; ?>" class="span4" /></div></div>

      <div class="row"><div class="healthcell"><?php print $text_realname; ?>:</div><div class="healthcell"><input type="text" name="realname" value="<?php print $user['realname']; ?>" class="span4" /></div></div>

      <div class="row">
       <div class="healthcell"><?php print $text_domain; ?>:</div><div class="healthcell">
         <select name="domain" class="span4">
<?php asort($domains); foreach ($domains as $domain) { ?>
            <option value="<?php print $domain; ?>"<?php if($domain == $user['domain']){ ?> selected="selected"<?php } ?>><?php print $domain; ?></option>
<?php } ?>
         </select>
       </div>
      </div>

<?php if(ENABLE_LDAP_IMPORT_FEATURE == 1) { ?>
      <div class="row"><div class="healthcell">LDAP DN:</div><div class="healthcell"><input type="text" name="dn" value="<?php print $user['dn']; ?>" class="span4" /></div><div class="healthcell"><?php print $text_dn_asterisk_means_skip_sync; ?></div></div>
<?php } ?>

      <div class="row"><div class="healthcell"><?php print $text_password; ?>:</div><div class="healthcell"><input type="password" name="password" value="" class="span4" /></div></div>
      <div class="row"><div class="healthcell"><?php print $text_password_again; ?>:</div><div class="healthcell"><input type="password" name="password2" value="" class="span4" /></div></div>
      <div class="row"><div class="healthcell"><?php print $text_user_id; ?>:</div><div class="healthcell"><?php print $uid; ?></div></div>
      <div class="row"><div class="healthcell"><?php print $text_group_id; ?>:</div><div class="healthcell"><input type="text" name="gid" value="<?php print $user['gid']; ?>" class="span4" /></div></div>
      <div class="row">
       <div class="healthcell"><?php print $text_policy_group; ?>:</div><div class="healthcell">
         <select name="policy_group" class="span4">
            <option value="0"<?php if($user['policy_group'] == 0){ ?> selected="selected"<?php } ?>><?php print DEFAULT_POLICY; ?></option>
<?php foreach ($policies as $policy) { ?>
            <option value="<?php print $policy['policy_group']; ?>"<?php if($user['policy_group'] == $policy['policy_group']){ ?> selected="selected"<?php } ?>><?php print $policy['name']; ?></option>
<?php } ?>
         </select>
       </div>
      </div>
      <div class="row">
       <div class="healthcell"><?php print $text_admin_user; ?>:</div><div class="healthcell">
         <select name="isadmin" class="span4">
            <option value="0"<?php if($user['isadmin'] == 0){ ?> selected="selected"<?php } ?>><?php print $text_user_regular; ?></option>
            <?php if(Registry::get('admin_user') == 1) { ?><option value="1"<?php if($user['isadmin'] == 1){ ?> selected="selected"<?php } ?>><?php print $text_user_masteradmin; ?></option><?php } ?>
            <option value="2"<?php if($user['isadmin'] == 2){ ?> selected="selected"<?php } ?>><?php print $text_user_domainadmin; ?></option>
            <option value="3"<?php if($user['isadmin'] == 3){ ?> selected="selected"<?php } ?>><?php print $text_user_read_only_admin; ?></option>
         </select>
       </div>
      </div>
      <div class="row"><div class="healthcell"><?php print $text_whitelist; ?>:</div><div class="healthcell"><textarea name="whitelist" class="span4"><?php print $user['whitelist']; ?></textarea></div></div>
      <div class="row"><div class="healthcell"><?php print $text_blacklist; ?>:</div><div class="healthcell"><textarea name="blacklist" class="span4"><?php print $user['blacklist']; ?></textarea></div></div>

      <div class="row">
         <div class="healthcell"><?php print $text_group_membership; ?>:</div>
         <div class="healthcell">
            <?php foreach ($user['group_membership'] as $_group_uid) { ?>
               <?php $a = preg_split("/\s/", $this->model_user_user->getEmailsByUid($_group_uid)); print $a[0]; ?></br />
            <?php } ?>
         </div>
      </div>

      <div class="row"><div class="healthcell">&nbsp;</div><div class="healthcell"><input type="submit" value="<?php print $text_modify; ?>" class="btn btn-primary" /><input type="reset" value="<?php print $text_cancel; ?>" class="btn btn-ok" /></div></div>
</form>

<p>&nbsp;</p>
<p><a href="index.php?route=user/remove&amp;confirmed=1&amp;uid=<?php print $user['uid']; ?>&amp;user=<?php print $user['username']; ?>" onclick="if(confirm('<?php print $text_remove_this_user; ?>: ' + '\'<?php print $user['username']; ?>\'')) return true; return false;"><?php print $text_remove_this_user; ?>: <?php print $user['username']; ?></a></p>
<p>&nbsp;</p>

<p>
<?php } else if(isset($x)){ print $x; ?>. 
<?php } ?>

<a href="index.php?route=user/list"><?php print $text_back; ?></a>
</p>
