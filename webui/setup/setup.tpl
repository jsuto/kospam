<script type="text/javascript">

function fix1() {

   if(document.forms.setup.DB_DRIVER.value == "sqlite3") {

      document.forms.setup.DB_HOSTNAME.value = "";
      document.forms.setup.DB_USERNAME.value = "";
      document.forms.setup.DB_DATABASE.value = "/var/lib/clapf/data/tokens.sdb";

      document.forms.setup.DB_HOSTNAME.disabled = 1;
      document.forms.setup.DB_USERNAME.disabled = 1;   
      document.forms.setup.DB_PASSWORD.disabled = 1;
   }

   if(document.forms.setup.DB_DRIVER.value == "mysql") {
      document.forms.setup.DB_HOSTNAME.disabled = 0;
      document.forms.setup.DB_USERNAME.disabled = 0;
      document.forms.setup.DB_PASSWORD.disabled = 0;

      document.forms.setup.DB_HOSTNAME.value = "localhost";
      document.forms.setup.DB_USERNAME.value = "clapf";
      document.forms.setup.DB_DATABASE.value = "clapf";
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
      <td><input type="text" name="QUEUE_DIRECTORY" value="/var/lib/clapf/queue" /></td>
   </tr>

   <tr>
      <td>SMTP host: </td>
      <td><input type="text" name="SMTP_HOST" value="127.0.0.1" /></td>
   </tr>

   <tr>
      <td>SMTP port: </td>
      <td><input type="text" name="SMTP_PORT" value="10026" /></td>
   </tr>

   <tr>
      <td>clapf port: </td>
      <td><input type="text" name="CLAPF_PORT" value="10025" /></td>
   </tr>

   <tr>
      <td>SMTP domain: </td>
      <td><input type="text" name="SMTP_DOMAIN" value="yourdomain.com" /></td>
   </tr>

   <tr>
      <td>Database driver: </td>
      <td>
         <select name="DB_DRIVER" id="DB_DRIVER" onchange="fix1(); return false;">
            <option value="mysql">MySQL</option>
            <option value="sqlite3">SQLite3</option>
         </select>
      </td>
   </tr>

   <tr>
      <td>Database host: </td>
      <td><input type="text" name="DB_HOSTNAME" id="DB_HOSTNAME" value="" /></td>
   </tr>

   <tr>
      <td>Database name: </td>
      <td><input type="text" name="DB_DATABASE" id="DB_DATABASE" value="clapf" /></td>
   </tr>

   <tr>
      <td>Database user: </td>
      <td><input type="text" name="DB_USERNAME" id="DB_USERNAME" value="clapf" /></td>
   </tr>

   <tr>
      <td>Database password: </td>
      <td><input type="password" name="DB_PASSWORD" id="DB_PASSWORD" value="" /></td>
   </tr>

   <tr>
      <td colspan="2"><input type="submit" value="Submit"> <input type="reset" value="Cancel"> </td>
   </tr>

</table>

</form>

