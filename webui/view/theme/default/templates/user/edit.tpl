<div id="deleteconfirm-modal" class="modal hide fade">
  <div class="modal-header">
    <button type="button" class="close" data-dismiss="modal" role="dialog" aria-hidden="true"><i class="icon-remove"></i></button>
    <h3><?php print $text_confirm; ?> <?php print $text_delete; ?></h3>
  </div>
  <div class="modal-body">
    <p><?php print $text_user_delete_confirm_message; ?> <span id="name">ERROR</span>?</p>
  </div>
  <div class="modal-footer">
    <a href="#" class="btn" data-dismiss="modal" aria-hidden="true"><?php print $text_close; ?></a>
    <a href="index.php?route=user/remove&amp;uid=-1&amp;name=Error&amp;confirmed=0" class="btn btn-primary" id="id"><?php print $text_delete; ?></a>
  </div>
</div>

<?php if(isset($errorstring)){ ?><div class="alert alert-danger"><?php print $text_error; ?>: <?php print $errorstring; ?></div><?php } ?>

<?php if(isset($x)){ ?>

<div class="alert alert-success"><?php print $x; ?>.</div>
<p><a href="index.php?route=user/list"><i class="icon-circle-arrow-left"></i>&nbsp;<?php print $text_back; ?></a></p>

<?php } elseif(isset($user)) { ?>

<p><a href="index.php?route=user/list"><i class="icon-circle-arrow-left"></i>&nbsp;<?php print $text_back; ?></a> | <a href="index.php?route=user/remove&amp;id=<?php print $user['uid']; ?>&amp;user=<?php print $user['username']; ?>" class="confirm-delete" data-id="<?php print $user['uid']; ?>" data-name="<?php print $user['realname']; ?>"><i class="icon-remove-sign"></i>&nbsp;<?php print $text_remove_this_user; ?></a></p>

    <form action="index.php?route=user/edit" name="edituser" method="post" autocomplete="off" class="form-horizontal">
     <div class="control-group<?php if(isset($errors['email'])){ print " error"; } ?>">
        <input type="hidden" name="uid" value="<?php print $uid; ?>" />
        <label class="control-label" for="email"><?php print $text_email_addresses; ?>:</label>
            <div class="controls">
              <textarea name="email"><?php print $emails; ?></textarea>
              <?php if ( isset($errors['email']) ) { ?><span class="help-inline"><?php print $errors['email']; ?></span><?php } ?>
            </div>
        </div>
        
        <div class="control-group<?php if(isset($errors['username'])){ print " error"; } ?>">
            <label class="control-label" for="username"><?php print $text_username; ?>:</label>
            <div class="controls">
              <input type="text" name="username" value="<?php print $user['username']; ?>" class="text" />
              <?php if ( isset($errors['username']) ) { ?><span class="help-inline"><?php print $errors['username']; ?></span><?php } ?>
            </div>
        </div>
        
         <div class="control-group<?php if(isset($errors['realname'])){ print " error"; } ?>">
            <label class="control-label" for="realname"><?php print $text_realname; ?>:</label>
            <div class="controls">
              <input type="text" name="realname" value="<?php print $user['realname']; ?>" class="text" />
              <?php if ( isset($errors['realname']) ) { ?><span class="help-inline"><?php print $errors['realname']; ?></span><?php } ?>
            </div>
        </div>	
        
        <div class="control-group<?php if(isset($errors['domain'])){ print " error"; } ?>">
            <label class="control-label" for="domain"><?php print $text_domain; ?>:</label>
            <div class="controls">
              <select name="domain">
                   <?php asort($domains); foreach ($domains as $domain) { ?>
                      <option value="<?php print $domain; ?>"<?php if($domain == $user['domain']){ ?> selected="selected"<?php } ?>><?php print $domain; ?></option>
                   <?php } ?>
              </select>
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

    <?php if(ENABLE_LDAP_IMPORT_FEATURE == 1) { ?>
         <div class="control-group<?php if(isset($errors['dn'])){ print " error"; } ?>">
            <label class="control-label" for="dn">LDAP DN:</label>
            <div class="controls">
              <input type="text" name="dn" value="<?php print $user['dn']; ?>" class="text" /><br /> (<?php print $text_dn_asterisk_means_skip_sync; ?>)
            </div>
        </div>
    <?php } ?>

        <div class="control-group<?php if(isset($errors['password'])){ print " error"; } ?>">
            <label class="control-label" for="password"><?php print $text_password; ?>:</label>
            <div class="controls">
              <input type="password" name="password" value="" class="text" />
            </div>
        </div>	

        <div class="control-group<?php if(isset($errors['password2'])){ print " error"; } ?>">
            <label class="control-label" for="password2"><?php print $text_password_again; ?>:</label>
            <div class="controls">
              <input type="password" name="password2" value="" class="text" />
            </div>
        </div>	
        
        <div class="control-group">
             <label class="control-label" for="uiddisplay"><?php print $text_user_id; ?>:</label>
             <div class="controls">
                <input type="text" name="uiddisplay" value="<?php print $uid; ?>" class="text" disabled />
             </div>
        </div>

        <div class="control-group<?php if(isset($errors['gid'])){ print " error"; } ?>">
            <label class="control-label" for="gid">GID:</label>
            <div class="controls">
              <input type="text" name="gid" id="gid" value="<?php print $user['gid']; ?>" class="text" />
              <?php if ( isset($errors['gid']) ) { ?><span class="help-inline"><?php print $errors['gid']; ?></span><?php } ?>
            </div>
        </div>
        
        <div class="control-group">
            <label class="control-label" for="isadmin"><?php print $text_admin_user; ?>:</label>
            <div class="controls">
              <select name="isadmin">
                   <option value="0"<?php if(isset($user['isadmin']) && $user['isadmin'] == 0){ ?> selected="selected"<?php } ?>><?php print $text_user_regular; ?></option>
                   <?php if(Registry::get('admin_user') == 1) { ?><option value="1"<?php if(isset($user['isadmin']) && $user['isadmin'] == 1){ ?> selected="selected"<?php } ?>><?php print $text_user_masteradmin; ?></option><?php } ?>
              </select>
            </div>
        </div>	
        
        <div class="form-actions">
            <input type="submit" value="<?php print $text_modify; ?>" class="btn btn-primary" /> <a href="index.php?route=user/list" class="btn"><?php print $text_cancel; ?></a>
        </div>

    </form>

<?php } ?>
