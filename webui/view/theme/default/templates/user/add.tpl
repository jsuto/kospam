
<?php if(isset($errorstring)){ ?><div class="alert alert-error"><?php print $text_error; ?>: <?php print $errorstring; ?></div><?php } ?>

<?php if(isset($x)){ ?>

<div class="alert alert-success"><?php print $x; ?>.</div>
<p><a href="index.php?route=user/list"><i class="icon-circle-arrow-left"></i>&nbsp;<?php print $text_back; ?></a></p>

<?php } else { ?>

<p><a href="index.php?route=user/list"><i class="icon-circle-arrow-left"></i>&nbsp;<?php print $text_back; ?></a></p>

<form action="index.php?route=user/add" name="adduser" method="post" autocomplete="off" class="form-horizontal">

    <div class="control-group<?php if(isset($errors['email'])){ print " error"; } ?>">
        <input type="hidden" name="uid" value="<?php print $next_user_id; ?>" />
        <label class="control-label" for="email"><?php print $text_email_addresses; ?>:</label>
		<div class="controls">
		  <textarea name="email" id="email"><?php if(isset($post['email'])){ print $post['email']; } ?></textarea>
          <?php if ( isset($errors['email']) ) { ?><span class="help-inline"><?php print $errors['email']; ?></span><?php } ?>
		</div>
	</div>
	
	<div class="control-group<?php if(isset($errors['username'])){ print " error"; } ?>">
		<label class="control-label" for="username"><?php print $text_username; ?>:</label>
		<div class="controls">
		  <input type="text" name="username" id="username" <?php if(isset($post['username'])){ print 'value="'.$post['username'].'" '; } ?>class="text" />
          <?php if ( isset($errors['username']) ) { ?><span class="help-inline"><?php print $errors['username']; ?></span><?php } ?>
		</div>
	</div>
	
	<div class="control-group<?php if(isset($errors['realname'])){ print " error"; } ?>">
		<label class="control-label" for="realname"><?php print $text_realname; ?>:</label>
		<div class="controls">
		  <input type="text" name="realname" id="realname" <?php if(isset($post['realname'])){ print 'value="'.$post['realname'].'" '; } ?>class="text" />
          <?php if ( isset($errors['realname']) ) { ?><span class="help-inline"><?php print $errors['realname']; ?></span><?php } ?>
		</div>
	</div>	
	
	<div class="control-group<?php if(isset($errors['domain'])){ print " error"; } ?>">
		<label class="control-label" for="domain"><?php print $text_domain; ?>:</label>
		<div class="controls">
		  <select name="domain" id="domain">
               <?php asort($domains); foreach ($domains as $domain) { ?>
                  <option value="<?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?>"<?php if( (isset($post) && $domain == $post['domain']) || (!isset($post) && $domain == $last_domain) ){ ?> selected="selected"<?php } ?>><?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?></option>
               <?php } ?>
          </select>
          <?php if ( isset($errors['domain']) ) { ?><span class="help-inline"><?php print $errors['domain']; ?></span><?php } ?>
		</div>
	</div>	

        <div class="control-group<?php if(isset($errors['policy_group'])){ print " error"; } ?>">
            <label class="control-label" for="policy_group"><?php print $text_policy; ?>:</label>
            <div class="controls">
              <select name="policy_group">
                      <option value="0""<?php if(0 == $user['policy_group']){ ?> selected="selected"<?php } ?>>default</option>
                   <?php asort($policies); foreach ($policies as $policy) { ?>
                      <option value="<?php print $policy['id']; ?>"<?php if($policy['id'] == $user['policy_group']){ ?> selected="selected"<?php } ?>><?php print $policy['name']; ?></option>
                   <?php } ?>
              </select>
            </div>
        </div>

        <div class="control-group<?php if(isset($errors['gid'])){ print " error"; } ?>">
            <label class="control-label" for="gid">GID:</label>
            <div class="controls">
              <input type="text" name="gid" id="gid" value="<?php if(isset($user['gid'])) { print $user['gid']; } else { ?>0<?php } ?>" class="text" />
              <?php if ( isset($errors['gid']) ) { ?><span class="help-inline"><?php print $errors['gid']; ?></span><?php } ?>
            </div>
        </div>


<?php if(ENABLE_LDAP_IMPORT_FEATURE == 1) { ?>
	<div class="control-group<?php if(isset($errors['dn'])){ print " error"; } ?>">
		<label class="control-label" for="dn">LDAP DN:</label>
		<div class="controls">
		  <input type="text" name="dn" id="dn" <?php if(isset($post['dn'])){ print 'value="'.$post['dn'].'" '; } ?>class="text" />
          <?php if ( isset($errors['dn']) ) { ?><span class="help-inline"><?php print $errors['dn']; ?></span><?php } ?>
          <br /> (<?php print $text_dn_asterisk_means_skip_sync; ?>)
		</div>
	</div>
<?php } ?>

	<div class="control-group<?php if(isset($errors['password'])){ print " error"; } ?>">
		<label class="control-label" for="password"><?php print $text_password; ?>:</label>
		<div class="controls">
		  <input type="password" name="password" id="password" value="<?php if(isset($post['password'])){ print $post['password']; } ?>" class="text" />
          <?php if ( isset($errors['password']) ) { ?><span class="help-inline"><?php print $errors['password']; ?></span><?php } ?>
		</div>
	</div>	

	<div class="control-group<?php if(isset($errors['password2'])){ print " error"; } ?>">
		<label class="control-label" for="password2"><?php print $text_password_again; ?>:</label>
		<div class="controls">
		  <input type="password" name="password2" id="password2" value="<?php if(isset($post['password2'])){ print $post['password2']; } ?>" class="text" />
          <?php if ( isset($errors['password2']) ) { ?><span class="help-inline"><?php print $errors['password2']; ?></span><?php } ?>
		</div>
	</div>	
	
	<div class="control-group">
		<label class="control-label" for="isadmin"><?php print $text_admin_user; ?>:</label>
		<div class="controls">
		  <select name="isadmin" id="isadmin">
               <option value="0"<?php if(isset($post['isadmin']) && $post['isadmin'] == 0){ ?> selected="selected"<?php } ?>><?php print $text_user_regular; ?></option>
               <?php if(Registry::get('admin_user') == 1) { ?><option value="1"<?php if(isset($post['isadmin']) && $post['isadmin'] == 1){ ?> selected="selected"<?php } ?>><?php print $text_user_masteradmin; ?></option><?php } ?>
               <!--option value="2"<?php if(isset($post['isadmin']) && $post['isadmin'] == 2){ ?> selected="selected"<?php } ?>><?php print $text_user_auditor; ?></option-->
          </select>
		</div>
	</div>	
	
	<div class="alert alert-info">*: <?php print $text_min_2_chars; ?><br />**: <?php print $text_enter_one_group_per_line; ?></div>
	
	<div class="form-actions">
        <input type="submit" value="<?php print $text_add; ?>" class="btn btn-primary" /> <a href="index.php?route=user/list" class="btn"><?php print $text_cancel; ?></a>
	</div>

</form>

<?php } ?>
