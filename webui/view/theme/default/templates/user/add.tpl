
<h4><?php print $text_add_new_user_alias; ?></h4>

<?php if(isset($errorstring)){ ?><p class="loginfailed"><?php print $text_error; ?>: <?php print $errorstring; ?></p><?php } ?>

<?php if(isset($policies)) { ?>

<form action="index.php?route=user/add" name="adduser" method="post" autocomplete="off">
      <div class="row"><div class="healthcell"><?php print $text_email_addresses; ?>:</div><div class="healthcell"><textarea name="email" class="span4"><?php if(isset($post['email'])){ print $post['email']; } ?></textarea></div></div>
      <div class="row"><div class="healthcell"><?php print $text_username; ?>:</div><div class="healthcell"><input type="text" name="username" value="<?php if(isset($post['username'])){ print $post['username']; } ?>" class="span4" /></div></div>
      <div class="row"><div class="healthcell"><?php print $text_realname; ?>:</div><div class="healthcell"><input type="text" name="realname" value="<?php if(isset($post['realname'])){ print $post['realname']; } ?>" class="span4" /></div></div>
      <div class="row">
       <div class="healthcell"><?php print $text_domain; ?>:</div><div class="healthcell">
         <select name="domain" class="span4">
<?php asort($domains); foreach ($domains as $domain) { ?>
            <option value="<?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?>"<?php if( (isset($post) && $domain == $post['domain']) || (!isset($post) && isset($_SESSION['last_domain']) && $domain == $_SESSION['last_domain']) ){ ?> selected="selected"<?php } ?>><?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?></option>
<?php } ?>
         </select>
       </div>
      </div>

<?php if(ENABLE_LDAP_IMPORT_FEATURE == 1) { ?>
      <div class="row"><div class="healthcell">LDAP DN:</div><div class="healthcell"><input type="text" name="dn" value="" class="span4" /></div><div class="healthcell"><?php print $text_dn_asterisk_means_skip_sync; ?></div></div>
<?php } ?>

      <div class="row"><div class="healthcell"><?php print $text_password; ?>:</div><div class="healthcell"><input type="password" name="password" value="" class="span4" /></div></div>
      <div class="row"><div class="healthcell"><?php print $text_password_again; ?>:</div><div class="healthcell"><input type="password" name="password2" value="" class="span4" /></div></div>
      <div class="row"><div class="healthcell"><?php print $text_user_id; ?>:</div><div class="healthcell"><input type="text" name="uid" value="<?php print $next_user_id; ?>" /></div></div>
      <div class="row"><div class="healthcell"><?php print $text_group_id; ?>:</div><div class="healthcell"><input type="text" name="gid" value="<?php if(isDomainAdmin() == 1) { print $_SESSION['gid']; } else { print $next_user_id; } ?>" /></div></div>
      <div class="row">
       <div class="healthcell"><?php print $text_policy_group; ?>:</div><div class="healthcell">
         <select name="policy_group" class="span4">
            <option value="0"<?php if(isset($post) && $post['policy_group'] == 0){ ?> selected="selected"<?php } ?>><?php print DEFAULT_POLICY; ?></option>
<?php foreach ($policies as $policy) { ?>
            <option value="<?php print $policy['policy_group']; ?>"<?php if(isset($post) && $post['policy_group'] == $policy['policy_group']){ ?> selected="selected"<?php } ?>><?php print $policy['name']; ?></option>
<?php } ?>
         </select>
       </div>
      </div>
      <div class="row">
       <div class="healthcell"><?php print $text_admin_user; ?>:</div><div class="healthcell">
         <select name="isadmin" class="span4">
            <option value="0"<?php if(isset($post) && $post['isadmin'] == 0){ ?> selected="selected"<?php } ?>><?php print $text_user_regular; ?></option>
            <?php if(Registry::get('admin_user') == 1) { ?><option value="1"<?php if(isset($post) && $post['isadmin'] == 1){ ?> selected="selected"<?php } ?>><?php print $text_user_masteradmin; ?></option><?php } ?>
            <option value="2"<?php if(isset($post) && $post['isadmin'] == 2){ ?> selected="selected"<?php } ?>><?php print $text_user_domainadmin; ?></option>
         </select>
       </div>
      </div>
      <div class="row"><div class="healthcell"><?php print $text_whitelist; ?>:</div><div class="healthcell"><textarea name="whitelist" class="span4"><?php if(isset($post['whitelist'])){ print $post['whitelist']; } ?></textarea></div></div>
      <div class="row"><div class="healthcell"><?php print $text_blacklist; ?>:</div><div class="healthcell"><textarea name="blacklist" class="span4"><?php if(isset($post['blacklist'])){ print $post['blacklist']; } ?></textarea></div></div>
      <div class="row"><div class="healthcell">&nbsp;</div><div class="healthcell"><input type="submit" value="<?php print $text_add; ?>" class="btn btn-primary" /></div></div>
</form>


<?php } else if(isset($x)){ print $x; ?>. <a href="index.php?route=user/list"><?php print $text_back; ?></a>
<?php } ?>

