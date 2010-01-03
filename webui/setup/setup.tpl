<script type="text/javascript">

function fix1() {

   if(document.forms.setup.DB_DRIVER.value == "mysql") {
      ShowOption('DB_HOSTNAME', 1);
      ShowOption('DB_DATABASE', 1);
      ShowOption('DB_USERNAME', 1);
      ShowOption('DB_PASSWORD', 1);

      document.forms.setup.DB_HOSTNAME.disabled = 0;
      document.forms.setup.DB_USERNAME.disabled = 0;
      document.forms.setup.DB_PASSWORD.disabled = 0;

      document.forms.setup.DB_HOSTNAME.value = "localhost";
      document.forms.setup.DB_USERNAME.value = "clapf";
      document.forms.setup.DB_DATABASE.value = "clapf";
   }
   else {
      ShowOption('DB_HOSTNAME', 0);
      ShowOption('DB_DATABASE', 0);
      ShowOption('DB_USERNAME', 0);
      ShowOption('DB_PASSWORD', 0);
   }

   if(document.forms.setup.DB_DRIVER.value == "sqlite3") {

      ShowOption('DB_HOSTNAME', 0);
      ShowOption('DB_DATABASE', 1);
      ShowOption('DB_USERNAME', 0);
      ShowOption('DB_PASSWORD', 0);

      document.forms.setup.DB_DATABASE.value = "/var/lib/clapf/data/tokens.sdb";
   }

   if(document.forms.setup.DB_DRIVER.value == "ldap") {
      ShowOption('LDAP_HOST', 1);
      ShowOption('LDAP_BINDDN', 1);
      ShowOption('LDAP_BINDPW', 1);
      ShowOption('LDAP_USER_BASEDN', 1);
      ShowOption('LDAP_POLICY_BASEDN', 1);
      ShowOption('LDAP_DOMAIN_BASEDN', 1);
   }
   else {
      ShowOption('LDAP_HOST', 0);
      ShowOption('LDAP_BINDDN', 0);
      ShowOption('LDAP_BINDPW', 0);
      ShowOption('LDAP_USER_BASEDN', 0);
      ShowOption('LDAP_POLICY_BASEDN', 0);
      ShowOption('LDAP_DOMAIN_BASEDN', 0);
   }


   if(document.forms.setup.HISTORY_DRIVER.value == "sqlite3") {
      document.forms.setup.HISTORY_DATABASE.value = "/var/lib/clapf/data/log.sdb";
   }
   else {
      document.forms.setup.HISTORY_DATABASE.value = "clapf";
   }

}


function ShowOption(what, value) {
   if(value == 1) {
      document.getElementById('DIV_'+what).style.display = '';
   } else {
      document.getElementById('DIV_'+what).style.display = 'none';
   }
                        
}


</script>


<form action="setup.php" method="post" name="setup">

<table border="0" align="middle">
   <tr>
      <td>Language: </td>
      <td>
         <select name="LANG">
            <option value="en">English</option>
            <option value="hu">Hungarian</option>
         </select>
      </td>
   </tr>

   <tr>
      <td>Theme: </td>
      <td>
         <select name="THEME">
            <option value="default">default</option>
         </select>
      </td>
   </tr>

   <tr>
      <td>Queue directory: </td>
      <td><input type="text" name="QUEUE_DIRECTORY" value="/var/lib/clapf/queue" size="30" /></td>
   </tr>

   <tr>
      <td>Queue directory splitting: </td>
      <td>
          <select name="QUEUE_DIR_SPLITTING" id="QUEUE_DIR_SPLITTING">
             <option value="0">by username</option>
             <option value="1">by uid</option>
          </select>
      </td>
   </tr>

   <tr>
      <td>SMTP host: </td>
      <td><input type="text" name="SMTP_HOST" value="127.0.0.1" size="30" /></td>
   </tr>

   <tr>
      <td>SMTP port: </td>
      <td><input type="text" name="SMTP_PORT" value="10026" size="30" /></td>
   </tr>

   <tr>
      <td>clapf port: </td>
      <td><input type="text" name="CLAPF_PORT" value="10025" size="30" /></td>
   </tr>

   <tr>
      <td>SMTP domain: </td>
      <td><input type="text" name="SMTP_DOMAIN" value="yourdomain.com" size="30" /></td>
   </tr>

   <tr>
      <td>Database driver: </td>
      <td>
         <select name="DB_DRIVER" id="DB_DRIVER" onchange="fix1(); return false;">
            <option value="mysql">MySQL</option>
            <option value="sqlite3">SQLite3</option>
            <option value="ldap">LDAP</option>
         </select>
      </td>
   </tr>

   <tr id="DIV_DB_HOSTNAME" style="display:show">
      <td>Database host: </td>
      <td><input type="text" name="DB_HOSTNAME" id="DB_HOSTNAME" value="localhost" size="30" /></td>
   </tr>

   <tr id="DIV_DB_DATABASE" style="display:show">
      <td>Database name: </td>
      <td><input type="text" name="DB_DATABASE" id="DB_DATABASE" value="clapf" size="30" /></td>
   </tr>

   <tr id="DIV_DB_USERNAME" style="display:show">
      <td>Database user: </td>
      <td><input type="text" name="DB_USERNAME" id="DB_USERNAME" value="clapf" size="30" /></td>
   </tr>

   <tr id="DIV_DB_PASSWORD" style="display:show">
      <td>Database password: </td>
      <td><input type="password" name="DB_PASSWORD" id="DB_PASSWORD" value="" size="30" /></td>
   </tr>

   <!-- ldap stuff -->

   <tr id="DIV_LDAP_HOST" style="display:none">
      <td>LDAP host: </td>
      <td><input type="text" name="LDAP_HOST" id="LDAP_HOST" value="ldap://127.0.0.1/" size="30" /></td>
   </tr>

   <tr id="DIV_LDAP_BINDDN" style="display:none">
      <td>Bind DN : </td>
      <td><input type="text" name="LDAP_BINDDN" id="LDAP_BINDDN" value="cn=Manager,dc=yourdomain.dc=com" size="30" /></td>
   </tr>

   <tr id="DIV_LDAP_BINDPW" style="display:none">
      <td>Bind password: </td>
      <td><input type="password" name="LDAP_BINDPW" id="LDAP_BINDPW" value="" size="30" /></td>
   </tr>

   <tr id="DIV_LDAP_USER_BASEDN" style="display:none">
      <td>Base DN of the users: </td>
      <td><input type="text" name="LDAP_USER_BASEDN" id="LDAP_USER_BASEDN" value="ou=clapfusers,dc=yourdomain.dc=com" size="30" /></td>
   </tr>

   <tr id="DIV_LDAP_POLICY_BASEDN" style="display:none">
      <td>Base DN of the policies: </td>
      <td><input type="text" name="LDAP_POLICY_BASEDN" id="LDAP_POLICY_BASEDN" value="ou=clapfpolicies,dc=yourdomain.dc=com" size="30" /></td>
   </tr>

   <tr id="DIV_LDAP_DOMAIN_BASEDN" style="display:none">
      <td>Base DN of the domains: </td>
      <td><input type="text" name="LDAP_DOMAIN_BASEDN" id="LDAP_DOMAIN_BASEDN" value="ou=clapfdomains,dc=yourdomain.dc=com" size="30" /></td>
   </tr>


   <!-- history stuff -->

   <tr>
      <td>History driver: </td>
      <td>
         <select name="HISTORY_DRIVER" id="HISTORY_DRIVER" onchange="fix1(); return false;">
            <option value="mysql">MySQL</option>
            <option value="sqlite3">SQLite3</option>
         </select>
      </td>
   </tr>

   <tr id="DIV_HISTORY_DATABASE" style="display:show">
      <td>History database name: </td>
      <td><input type="text" name="HISTORY_DATABASE" id="HISTORY_DATABASE" value="clapf" size="30" /></td>
   </tr>

   <tr>
      <td colspan="2"><input type="submit" value="Submit"> <input type="reset" value="Cancel"> </td>
   </tr>

</table>

</form>

