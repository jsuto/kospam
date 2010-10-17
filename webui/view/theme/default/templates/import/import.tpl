
<p>&nbsp;</p>

<?php if(isset($users) && count($users) > 1){ ?>

<form method="post" name="massedit" action="index.php?route=import/import">

<input type="hidden" name="ldap_host" value="<?php print $request['ldap_host']; ?>" />
<input type="hidden" name="ldap_binddn" value="<?php print $request['ldap_binddn']; ?>" />
<input type="hidden" name="ldap_bindpw" value="<?php print $request['ldap_bindpw']; ?>" />
<input type="hidden" name="ldap_basedn" value="<?php print $request['ldap_basedn']; ?>" />
<input type="hidden" name="type" value="<?php print $request['type']; ?>" />
<input type="hidden" name="domain" value="<?php print $request['domain']; ?>" />

<table border="0">
   <tr>
      <td><?php print $text_domainname; ?>:</td><td>
         <select name="domain">
<?php foreach ($domains as $domain) { ?>
            <option value="<?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?>"><?php if(is_array($domain)){ print $domain['mapped']; } else { print $domain; } ?></option>
<?php } ?>
         </select>
      </td>
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
   <tr>
      <td>GID:</td>
      <td><input type="text" name="gid" value="1" /></td>
   </tr>
</table>

<table border="1">
   <tr align="center">
      <th><?php print $text_exclude; ?></th>
      <th><?php print $text_username; ?></th>
      <th><?php print $text_email_addresses; ?></th>
   </tr>

<?php $i = 0; foreach($users as $user) { $i++; ?>
   <tr align="left" valign="top">
      <td><input type="checkbox" name="reject_<?php print $i; ?>" value="<?php print $user['dn']; ?>" /></td>
      <td><?php print $user['dn']; ?></td>
      <td><?php print $user['emails']; ?></td>
   </tr>
<?php } ?>

</table>



<input type="submit" value="<?php print $text_import_users; ?>" /></form>

<?php } else { ?>
<?php print $text_not_found; ?>
<?php } ?>


