
<p>&nbsp;</p>

<form method="post" name="massedit" action="index.php?route=import/query">

<table border="1">
   <tr>
      <td><?php print $text_ldap_host; ?>:</td>
      <td><input type="text" name="ldap_host" value="<?php print @$ldap['remotehost']; ?>" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td>
   </tr>
   <tr>
      <td><?php print $text_ldap_basedn; ?>:</td>
      <td><input type="text" name="ldap_basedn" value="<?php print @$ldap['basedn']; ?>" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td>
   </tr>
   <tr>
      <td><?php print $text_ldap_binddn; ?>:</td>
      <td><input type="text" name="ldap_binddn" value="<?php print @$ldap['binddn']; ?>" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td>
   </tr>
   <tr>
      <td><?php print $text_ldap_bindpw; ?>:</td>
      <td><input type="password" name="ldap_bindpw" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td>
   </tr>
   <tr>
      <td><?php print $text_ldap_type; ?></td>
      <td>Active Directory <input type="radio" name="type" value="AD" />, openldap <input type="radio" name="type" value="openldap" checked="checked"/> </td>
   <tr>
</table>

<input type="submit" value="<?php print $text_import_users; ?>" /></form>


