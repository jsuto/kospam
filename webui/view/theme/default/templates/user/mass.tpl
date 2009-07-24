
<p/>

<h4><?php print $text_bulk_update_selected_uids; ?></h4>

<?php if(isset($policies)) { ?>

<form action="index.php?route=user/massedit" name="massedit" method="post">
   <input type="hidden" name="edit" value="1" />
<?php foreach ($uids as $uid) { ?>
   <input type="hidden" name="aa_<?php print $uid; ?>" value="1" />
<?php } ?>

   <table border="0">
      <tr>
         <td><?php print $text_uids; ?>: <?php print $uidlist; ?></td>
      </tr>
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
      <tr valign="top"><td><?php print $text_whitelist; ?>:</td><td><textarea name="whitelist" cols="30" rows="5"></textarea></td></tr>
      <tr valign="top"><td><?php print $text_blacklist; ?>:</td><td><textarea name="blacklist" cols="30" rows="5"></textarea></td></tr>
      <tr colspan="2"><td><input type="submit" value="<?php print $text_update_selected_uids; ?>"></td></tr>
   </table>
</form>



<h4><?php print $text_remove_selected_uids; ?></h4>

<p><?php print $text_uids; ?>: <?php print $uidlist; ?></p>


<p>
<form action="index.php?route=user/massedit" name="massremove" method="post">
   <input type="hidden" name="remove" value="1" />
<?php foreach ($uids as $uid) { ?>
   <input type="hidden" name="aa_<?php print $uid; ?>" value="1" />
<?php } ?>
   <input type="submit" value="<?php print $text_remove_selected_uids; ?>" />
</form>
</p>


<?php } else if(isset($x)){ print $x; ?>. <a href="index.php?route=user/list"><?php print $text_back; ?></a>
<?php } ?>


