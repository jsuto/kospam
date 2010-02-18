<h4><?php print $text_existing_policies; ?>*</h4>

<form action="index.php?route=policy/view" method="get" onsubmit="fix_form(); return false;">
   <select name="policy_group">
<?php foreach ($policies as $policy) { ?>
      <option value="<?php print $policy['policy_group']; ?>"><?php print $policy['name']; ?></option>
<?php } ?>
   </select>

   <input type="submit" value="<?php print $text_edit_or_view; ?>" />
</form>

<p>&nbsp; </p>

<p><a href="index.php?route=policy/add"><?php print $text_add_policy; ?></a></p>

<p>&nbsp; </p>

<p>*: <?php print $text_warning_about_default_policy; ?></p>

