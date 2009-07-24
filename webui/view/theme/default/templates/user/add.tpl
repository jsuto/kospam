
<p/>

<h4><?php print $text_add_new_user_alias; ?></h4>

<?php if(isset($policies)) { ?>

<form action="index.php?route=user/add" name="adduser" method="post">
   <table border="0">
      <tr><td><?php print $text_email; ?>:</td><td><input type="text" name="email" value=""></td></tr>
      <tr><td><?php print $text_username; ?>:</td><td><input type="text" name="username" value=""></td></tr>
      <tr><td><?php print $text_password; ?>:</td><td><input type="password" name="password" value=""></td></tr>
      <tr><td><?php print $text_password_again; ?>:</td><td><input type="password" name="password2" value=""></td></tr>
      <tr><td><?php print $text_user_id; ?>:</td><td><input type="text" name="uid" value="<?php print $next_user_id; ?>"></td></tr>
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
            <option value="0" selected>0</option>
            <option value="1">1</option>
         </select>
       </td>
      </tr>
      <tr valign="top"><td><?php print $text_whitelist; ?>:</td><td><textarea name="whitelist" cols="30" rows="5"></textarea></td></tr>
      <tr valign="top"><td><?php print $text_blacklist; ?>:</td><td><textarea name="blacklist" cols="30" rows="5"></textarea></td></tr>
      <tr colspan="2"><td><input type="submit" value="<?php print $text_add; ?>"></td></tr>
   </table>
</form>


<?php } else if(isset($x)){ print $x; ?>. <a href="index.php?route=user/list"><?php print $text_back; ?></a>
<?php } ?>


