
<p/>

<?php if(isset($user)) { ?>

<form action="index.php?route=user/edit" name="adduser" method="post">
   <input type="hidden" name="uid" value="<?php print $uid; ?>">
   <input type="hidden" name="email_orig" value="<?php print $email; ?>">

   <table border="0">
      <tr><td><?php print $text_email; ?>:</td><td><input type="text" name="email" value="<?php print $email; ?>"></td></tr>
<?php if(DB_DRIVER == 'ldap') { ?>
      <tr valign="top"><td><?php print $text_email_aliases; ?>:</td><td><textarea name="mailalternateaddress" cols="30" rows="5"><?php print $user['aliases']; ?></textarea></td></tr>
<?php } ?>
      <tr><td><?php print $text_username; ?>:</td><td><input type="text" name="username" value="<?php print $user['username']; ?>"></td></tr>
      <tr><td><?php print $text_password; ?>:</td><td><input type="password" name="password" value=""></td></tr>
      <tr><td><?php print $text_password_again; ?>:</td><td><input type="password" name="password2" value=""></td></tr>
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
            <option value="0"<?php if($user['isadmin'] == 0){ ?> selected="selected"<?php } ?>>0</option>
            <option value="1"<?php if($user['isadmin'] == 1){ ?> selected="selected"<?php } ?>>1</option>
         </select>
       </td>
      </tr>
      <tr valign="top"><td><?php print $text_whitelist; ?>:</td><td><textarea name="whitelist" cols="30" rows="5"><?php print $user['whitelist']; ?></textarea></td></tr>
      <tr valign="top"><td><?php print $text_blacklist; ?>:</td><td><textarea name="blacklist" cols="30" rows="5"><?php print $user['blacklist']; ?></textarea></td></tr>
      <tr><td>&nbsp;</td><td><input type="submit" value="<?php print $text_modify; ?>"><input type="reset" value="<?php print $text_cancel; ?>"></td></tr>
   </table>
</form>

<p>&nbsp;</p>
<p><a href="index.php?route=user/remove&uid=<?php print $user['uid']; ?>&email=<?php print $email; ?>">Remove this user/alias</a></p>
<p>&nbsp;</p>

<p>
<?php } else if(isset($x)){ print $x; ?>. 
<?php } ?>

<a href="index.php?route=user/list"><?php print $text_back; ?></a>
</p>
