
<p>&nbsp;</p>

<form method="post" name="massedit" action="index.php?route=import/query">

<table border="1">
   <tr>
      <td><?php print $text_ldap_host; ?>:</td>
      <td><input type="text" name="ldap_host" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td>
   </tr>
   <tr>
      <td><?php print $text_ldap_binddn; ?>:</td>
      <td><input type="text" name="ldap_binddn" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td>
   </tr>
   <tr>
      <td><?php print $text_ldap_bindpw; ?>:</td>
      <td><input type="password" name="ldap_bindpw" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td>
   </tr>
   <tr>
      <td><?php print $text_ldap_basedn; ?>:</td>
      <td><input type="text" name="ldap_basedn" value="" size="<?php print CGI_INPUT_FIELD_WIDTH; ?>" /></td>
   </tr>

</table>

<input type="submit" value="<?php print $text_import_users; ?>" /></form>


