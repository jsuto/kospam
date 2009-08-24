
<p/>

<h4><?php print $text_add_new_user_alias; ?></h4>

<?php if(isset($policies)) { ?>

<form action="index.php?route=user/add" name="adduser" method="post">
   <table border="0">
<?php if(DB_DRIVER == 'ldap') { ?>
      <tr><td><?php print $text_email; ?>:</td><td><input type="text" name="email" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr valign="top"><td><?php print $text_email_aliases; ?>:</td><td><textarea name="mailalternateaddress" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"></textarea></td></tr>
<?php } else { ?>
      <tr valign="top"><td><?php print $text_email_addresses; ?>:</td><td><textarea name="email" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"></textarea></td></tr>
<?php } ?>
      <tr><td><?php print $text_username; ?>:</td><td><input type="text" name="username" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr><td><?php print $text_password; ?>:</td><td><input type="password" name="password" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr><td><?php print $text_password_again; ?>:</td><td><input type="password" name="password2" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td></tr>
      <tr><td><?php print $text_user_id; ?>:</td><td><input type="text" name="uid" value="<?php print $next_user_id; ?>" /></td></tr>
      <tr>
       <td><?php print $text_policy_group; ?>:</td><td>
         <select name="policy_group">
            <option value="0"><?php print DEFAULT_POLICY; ?></option>
<?php foreach ($policies as $policy) { ?>
            <option value="<?php print $policy['policy_group']; ?>"><?php print $policy['name']; ?></option>
<?php } ?>
         </select>
       </td>
      </tr>
      <tr>
       <td><?php print $text_admin_user; ?>:</td><td>
         <select name="isadmin">
            <option value="0" selected="selected">0</option>
            <option value="1">1</option>
         </select>
       </td>
      </tr>
      <tr valign="top"><td><?php print $text_whitelist; ?>:</td><td><textarea name="whitelist" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"></textarea></td></tr>
      <tr valign="top"><td><?php print $text_blacklist; ?>:</td><td><textarea name="blacklist" cols="<?php print CGI_INPUT_FIELD_WIDTH; ?>" rows="<?php print CGI_INPUT_FIELD_HEIGHT; ?>"></textarea></td></tr>
      <tr><td colspan="2"><input type="submit" value="<?php print $text_add; ?>" /></td></tr>
   </table>
</form>


<?php } else if(isset($x)){ print $x; ?>. <a href="index.php?route=user/list"><?php print $text_back; ?></a>
<?php } ?>


