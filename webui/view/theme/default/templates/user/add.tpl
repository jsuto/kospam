
<h4><?php print $text_add_new_user_alias; ?></h4>

<?php if(isset($errorstring)){ ?><p class="loginfailed"><?php print $text_error; ?>: <?php print $errorstring; ?></p><?php } ?>

<?php if(isset($policies)) { ?>

<form action="index.php?route=user/add" name="adduser" method="post">
   <table border="0">
<?php if(DB_DRIVER == 'ldap') { ?>
      <tr><td><?php print $text_email; ?>:</td><td><input type="text" name="email" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr valign="top"><td><?php print $text_email_aliases; ?>:</td><td><textarea name="mailalternateaddress" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"></textarea></td></tr>
<?php } else { ?>
      <tr valign="top"><td><?php print $text_email_addresses; ?>:</td><td><textarea name="email" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"><?php if(isset($post['email'])){ print $post['email']; } ?></textarea></td></tr>
<?php } ?>
      <tr><td><?php print $text_username; ?>:</td><td><input type="text" name="username" value="<?php if(isset($post['username'])){ print $post['username']; } ?>" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr>
       <td><?php print $text_domain; ?>:</td><td>
         <select name="domain">
<?php foreach ($domains as $domain) { ?>
            <option value="<?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?>"<?php if(isset($post) && $domain == $post['domain']){ ?> selected="selected"<?php } ?>><?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?></option>
<?php } ?>
         </select>
       </td>
      </tr>

<?php if(ENTERPRISE_VERSION == 1) { ?>
      <tr><td>LDAP DN:</td><td><input type="text" name="dn" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td><td><?php print $text_dn_asterisk_means_skip_sync; ?></td></tr>
<?php } ?>

      <tr><td><?php print $text_password; ?>:</td><td><input type="password" name="password" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr><td><?php print $text_password_again; ?>:</td><td><input type="password" name="password2" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr><td><?php print $text_user_id; ?>:</td><td><input type="text" name="uid" value="<?php print $next_user_id; ?>" /></td></tr>
      <tr>
       <td><?php print $text_policy_group; ?>:</td><td>
         <select name="policy_group">
            <option value="0"<?php if(isset($post) && $post['policy_group'] == 0){ ?> selected="selected"<?php } ?>><?php print DEFAULT_POLICY; ?></option>
<?php foreach ($policies as $policy) { ?>
            <option value="<?php print $policy['policy_group']; ?>"<?php if(isset($post) && $post['policy_group'] == $policy['policy_group']){ ?> selected="selected"<?php } ?>><?php print $policy['name']; ?></option>
<?php } ?>
         </select>
       </td>
      </tr>
      <tr>
       <td><?php print $text_admin_user; ?>:</td><td>
         <select name="isadmin">
            <option value="0"<?php if(isset($post) && $post['isadmin'] == 0){ ?> selected="selected"<?php } ?>><?php print $text_user_regular; ?></option>
            <?php if(Registry::get('admin_user') == 1) { ?><option value="1"<?php if(isset($post) && $post['isadmin'] == 1){ ?> selected="selected"<?php } ?>><?php print $text_user_masteradmin; ?></option><?php } ?>
            <option value="2"<?php if(isset($post) && $post['isadmin'] == 2){ ?> selected="selected"<?php } ?>><?php print $text_user_domainadmin; ?></option>
         </select>
       </td>
      </tr>
      <tr valign="top"><td><?php print $text_whitelist; ?>:</td><td><textarea name="whitelist" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"><?php if(isset($post['whitelist'])){ print $post['whitelist']; } ?></textarea></td></tr>
      <tr valign="top"><td><?php print $text_blacklist; ?>:</td><td><textarea name="blacklist" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"><?php if(isset($post['blacklist'])){ print $post['blacklist']; } ?></textarea></td></tr>
      <tr><td colspan="2"><input type="submit" value="<?php print $text_add; ?>" /></td></tr>
   </table>
</form>


<?php } else if(isset($x)){ print $x; ?>. <a href="index.php?route=user/list"><?php print $text_back; ?></a>
<?php } ?>

