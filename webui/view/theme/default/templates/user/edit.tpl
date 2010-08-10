

<?php if(isset($user)) { ?>

<form action="index.php?route=user/edit" name="adduser" method="post">
   <input type="hidden" name="uid" value="<?php print $uid; ?>" />

   <table border="0">
<?php if(DB_DRIVER == 'ldap') { ?>
      <tr><td><?php print $text_email; ?>:</td><td><input type="text" name="email" value="<?php print $email; ?>" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr valign="top"><td><?php print $text_email_aliases; ?>:</td><td><textarea name="mailalternateaddress" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"><?php print $user['aliases']; ?></textarea></td></tr>
<?php } else { ?>
      <tr valign="top">
         <td><?php print $text_email_addresses; ?>:</td>
         <td><textarea name="email" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"><?php print $emails; ?></textarea></td>
      </tr>
<?php } ?>

      <tr><td><?php print $text_username; ?>:</td><td><input type="text" name="username" value="<?php print $user['username']; ?>" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>

      <tr><td><?php print $text_realname; ?>:</td><td><input type="text" name="realname" value="<?php print $user['realname']; ?>" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>

      <tr>
       <td><?php print $text_domain; ?>:</td><td>
         <select name="domain">
<?php foreach ($domains as $domain) { ?>
            <option value="<?php print $domain; ?>"<?php if($domain == $user['domain']){ ?> selected="selected"<?php } ?>><?php print $domain; ?></option>
<?php } ?>
         </select>
       </td>
      </tr>

<?php if(ENTERPRISE_VERSION == 1) { ?>
      <tr><td>LDAP DN:</td><td><input type="text" name="dn" value="<?php print $user['dn']; ?>" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td><td><?php print $text_dn_asterisk_means_skip_sync; ?></td></tr>
<?php } ?>

      <tr><td><?php print $text_password; ?>:</td><td><input type="password" name="password" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr><td><?php print $text_password_again; ?>:</td><td><input type="password" name="password2" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr><td><?php print $text_user_id; ?>:</td><td><?php print $uid; ?></td></tr>
      <tr>
       <td><?php print $text_policy_group; ?>:</td><td>
         <select name="policy_group">
            <option value="0"<?php if($user['policy_group'] == 0){ ?> selected="selected"<?php } ?>><?php print DEFAULT_POLICY; ?></option>
<?php foreach ($policies as $policy) { ?>
            <option value="<?php print $policy['policy_group']; ?>"<?php if($user['policy_group'] == $policy['policy_group']){ ?> selected="selected"<?php } ?>><?php print $policy['name']; ?></option>
<?php } ?>
         </select>
       </td>
      </tr>
      <tr>
       <td><?php print $text_admin_user; ?>:</td><td>
         <select name="isadmin">
            <option value="0"<?php if($user['isadmin'] == 0){ ?> selected="selected"<?php } ?>><?php print $text_user_regular; ?></option>
            <?php if(Registry::get('admin_user') == 1) { ?><option value="1"<?php if($user['isadmin'] == 1){ ?> selected="selected"<?php } ?>><?php print $text_user_masteradmin; ?></option><?php } ?>
            <option value="2"<?php if($user['isadmin'] == 2){ ?> selected="selected"<?php } ?>><?php print $text_user_domainadmin; ?></option>
         </select>
       </td>
      </tr>
      <tr valign="top"><td><?php print $text_whitelist; ?>:</td><td><textarea name="whitelist" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"><?php print $user['whitelist']; ?></textarea></td></tr>
      <tr valign="top"><td><?php print $text_blacklist; ?>:</td><td><textarea name="blacklist" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"><?php print $user['blacklist']; ?></textarea></td></tr>
      <tr><td>&nbsp;</td><td><input type="submit" value="<?php print $text_modify; ?>" /><input type="reset" value="<?php print $text_cancel; ?>" /></td></tr>
   </table>
</form>

<p>&nbsp;</p>
<p><a href="index.php?route=user/remove&amp;uid=<?php print $user['uid']; ?>&amp;user=<?php print $user['username']; ?>"><?php print $text_remove_this_user; ?>: <?php print $user['username']; ?></a></p>
<p>&nbsp;</p>

<p>
<?php } else if(isset($x)){ print $x; ?>. 
<?php } ?>

<a href="index.php?route=user/list"><?php print $text_back; ?></a>
</p>
