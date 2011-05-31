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

      document.forms.setup.DB_HOSTNAME.value = "<?php if(isset($a['DB_HOSTNAME'])) { print DB_HOSTNAME; } else { ?>localhost<?php } ?>";
      document.forms.setup.DB_USERNAME.value = "<?php if(isset($a['DB_USERNAME'])) { print DB_USERNAME; } else { ?>clapf<?php } ?>";
      document.forms.setup.DB_DATABASE.value = "<?php if(isset($a['DB_DATABASE'])) { print DB_DATABASE; } else { ?>clapf<?php } ?>";
   }
   else {
      ShowOption('DB_HOSTNAME', 0);
      ShowOption('DB_DATABASE', 0);
      ShowOption('DB_USERNAME', 0);
      ShowOption('DB_PASSWORD', 0);
   }

   if(document.forms.setup.DB_DRIVER.value == "sqlite") {

      ShowOption('DB_HOSTNAME', 0);
      ShowOption('DB_DATABASE', 1);
      ShowOption('DB_USERNAME', 0);
      ShowOption('DB_PASSWORD', 0);

      document.forms.setup.DB_DATABASE.value = "<?php if(isset($a['DB_DATABASE'])) { print DB_DATABASE; } else { ?>/var/lib/clapf/data/tokens.sdb<?php } ?>";
   }

   if(document.forms.setup.MEMCACHED_ENABLED.value == 1) {
      ShowOption('MEMCACHED_SERVERS', 1);
   }
   else {
      ShowOption('MEMCACHED_SERVERS', 0);
   }

}


function fix_history() {
   if(document.forms.setup.HISTORY_DRIVER.value == "mysql") {
      ShowOption('HISTORY_HOSTNAME', 1);
      ShowOption('HISTORY_USERNAME', 1);
      ShowOption('HISTORY_PASSWORD', 1);
      document.forms.setup.HISTORY_DATABASE.value = "<?php if(isset($a['HISTORY_DATABASE'])) { print HISTORY_DATABASE; } else { ?>clapf<?php } ?>";
   }
   else {
      ShowOption('HISTORY_HOSTNAME', 0);
      ShowOption('HISTORY_USERNAME', 0);
      ShowOption('HISTORY_PASSWORD', 0);
      document.forms.setup.HISTORY_DATABASE.value = "<?php if(isset($a['HISTORY_DATABASE'])) { print HISTORY_DATABASE; } else { ?>/var/lib/clapf/stat/history.sdb<?php } ?>";
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

<?php //print_r($a); ?>

<table border="0" align="middle">
   <tr>
      <td>Language: </td>
      <td>
         <select name="LANG">
            <option value="en"<?php if(LANG == "en" || (isset($_POST['LANG']) && $_POST['LANG'] == "en") ) { ?> selected="selected"<?php } ?>>English</option>
            <option value="hu"<?php if(LANG == "hu" || (isset($_POST['LANG']) && $_POST['LANG'] == "hu") ) { ?> selected="selected"<?php } ?>>Hungarian</option>
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

   <tr<?php if(isset($error['stat_directory'])) { ?> class="error"<?php } ?>>
      <td>Stat directory: </td>
      <td><input type="text" name="STAT_DIRECTORY" value="<?php if(isset($a['STAT_DIRECTORY'])) { print STAT_DIRECTORY; } else if(isset($_POST['STAT_DIRECTORY'])) { print $_POST['STAT_DIRECTORY']; } else { ?>/var/lib/clapf/stat<?php } ?>" size="30" /></td>
      <?php if(isset($error['stat_directory'])) { ?><td><?php print $error['stat_directory']; ?></td><?php } ?>
   </tr>

   <tr<?php if(isset($error['queue_directory'])) { ?> class="error"<?php } ?>>
      <td>Queue directory: </td>
      <td><input type="text" name="QUEUE_DIRECTORY" value="<?php if(isset($a['QUEUE_DIRECTORY'])) { print QUEUE_DIRECTORY; } else if(isset($_POST['QUEUE_DIRECTORY'])) { print $_POST['QUEUE_DIRECTORY']; } else { ?>/var/lib/clapf/queue<?php } ?>" size="30" /></td>
      <?php if(isset($error['queue_directory'])) { ?><td><?php print $error['queue_directory']; ?></td><?php } ?>
   </tr>

   <tr>
      <td>Queue directory splitting: </td>
      <td>
          <select name="QUEUE_DIR_SPLITTING" id="QUEUE_DIR_SPLITTING">
             <option value="0"<?php if(QUEUE_DIR_SPLITTING == 0 || (isset($_POST['QUEUE_DIR_SPLITTING']) && $_POST['QUEUE_DIR_SPLITTING'] == 0) ) { ?> selected="selected"<?php } ?>>by username</option>
             <option value="1"<?php if(QUEUE_DIR_SPLITTING == 1 || (isset($_POST['QUEUE_DIR_SPLITTING']) && $_POST['QUEUE_DIR_SPLITTING'] == 1) ) { ?> selected="selected"<?php } ?>>by uid</option>
          </select>
      </td>
   </tr>

   <tr>
      <td>Postfix listen address before content filter: </td>
      <td><input type="text" name="POSTFIX_LISTEN_ADDRESS" value="<?php if(isset($a['POSTFIX_LISTEN_ADDRESS'])) { print POSTFIX_LISTEN_ADDRESS; } else if(isset($_POST['POSTFIX_LISTEN_ADDRESS'])) { print $_POST['POSTFIX_LISTEN_ADDRESS']; } else { ?>0.0.0.0<?php } ?>" size="30" /></td>
   </tr>

   <tr>
      <td>Postfix listen port after content filter: </td>
      <td><input type="text" name="POSTFIX_PORT_AFTER_CONTENT_FILTER" value="<?php if(isset($a['POSTFIX_PORT_AFTER_CONTENT_FILTER'])) { print POSTFIX_PORT_AFTER_CONTENT_FILTER; } else if(isset($_POST['POSTFIX_PORT_AFTER_CONTENT_FILTER'])) { print $_POST['POSTFIX_PORT_AFTER_CONTENT_FILTER']; } else { ?>10026<?php } ?>" size="30" /></td>
   </tr>

   <tr>
      <td>clapf port: </td>
      <td><input type="text" name="CLAPF_PORT" value="<?php if(isset($a['CLAPF_PORT'])) { print CLAPF_PORT; } else if(isset($_POST['CLAPF_PORT'])) { print $_POST['CLAPF_PORT']; } else { ?>10025<?php } ?>" size="30" /></td>
   </tr>

   <tr>
      <td>SMTP domain: </td>
      <td><input type="text" name="SMTP_DOMAIN" value="<?php if(isset($a['SMTP_DOMAIN'])) { print SMTP_DOMAIN; } else if(isset($_POST['SMTP_DOMAIN'])) { print $_POST['SMTP_DOMAIN']; } else { ?>yourdomain.com<?php } ?>" size="30" /></td>
   </tr>

   <tr>
      <td colspan="2"><hr><br /><strong>User database</strong></td>
   </tr>

   <tr>
      <td>Database driver: </td>
      <td>
         <select name="DB_DRIVER" id="DB_DRIVER" onchange="fix1(); return false;">
<?php if(MYSQL_DRIVER == 1) { ?>
            <option value="mysql"<?php if(DB_DRIVER == "mysql" || (isset($_POST['DB_DRIVER']) && $_POST['DB_DRIVER'] == "mysql") ) { ?> selected="selected"<?php } ?>>MySQL</option>
<?php } ?>
<?php if(SQLITE_DRIVER == 1) { ?>
            <option value="sqlite"<?php if(DB_DRIVER == "sqlite" || (isset($_POST['DB_DRIVER']) && $_POST['DB_DRIVER'] == "sqlite") ) { ?> selected="selected"<?php } ?>>SQLite3</option>
<?php } ?>
         </select>
      </td>
   </tr>

   <tr id="DIV_DB_HOSTNAME" style="display:show">
      <td>Database host: </td>
      <td><input type="text" name="DB_HOSTNAME" id="DB_HOSTNAME" value="<?php if(isset($a['DB_HOSTNAME'])) { print DB_HOSTNAME; } else if(isset($_POST['DB_HOSTNAME'])) { print $_POST['DB_HOSTNAME']; } else { ?>localhost<?php } ?>" size="30" /></td>
   </tr>

   <tr id="DIV_DB_DATABASE" style="display:show"<?php if(isset($error['database'])) { ?> class="error"<?php } ?>>
      <td>Database name: </td>
      <td><input type="text" name="DB_DATABASE" id="DB_DATABASE" value="<?php if(isset($a['DB_DATABASE'])) { print DB_DATABASE; } else if(isset($_POST['DB_DATABASE'])) { print $_POST['DB_DATABASE']; } else { ?>clapf<?php } ?>" size="30" /></td>
      <?php if(isset($error['database'])) { ?><td><?php print $error['database']; ?></td><?php } ?>
   </tr>

   <tr id="DIV_DB_USERNAME" style="display:show">
      <td>Database user: </td>
      <td><input type="text" name="DB_USERNAME" id="DB_USERNAME" value="<?php if(isset($a['DB_USERNAME'])) { print DB_USERNAME; } else if(isset($_POST['DB_USERNAME'])) { print $_POST['DB_USERNAME']; } else { ?>clapf<?php } ?>" size="30" /></td>
   </tr>

   <tr id="DIV_DB_PASSWORD" style="display:show">
      <td>Database password: </td>
      <td><input type="password" name="DB_PASSWORD" id="DB_PASSWORD" value="<?php if(isset($a['DB_PASSWORD'])) { print DB_PASSWORD; } else if(isset($_POST['DB_PASSWORD'])) { print $_POST['DB_PASSWORD']; } ?>" size="30" /></td>
   </tr>


   <!-- history stuff -->

   <tr>
      <td colspan="2"><hr><br /><strong>History database</strong></td>
   </tr>

   <tr>
      <td>History driver: </td>
      <td>
         <select name="HISTORY_DRIVER" id="HISTORY_DRIVER" onchange="fix_history(); return false;">
<?php if(MYSQL_DRIVER == 1) { ?>
            <option value="mysql"<?php if(HISTORY_DRIVER == "mysql" || (isset($_POST['HISTORY_DRIVER']) && $_POST['HISTORY_DRIVER'] == "mysql") ) { ?> selected="selected"<?php } ?>>MySQL</option>
<?php } ?>
<?php if(SQLITE_DRIVER == 1) { ?>
            <option value="sqlite"<?php if(HISTORY_DRIVER == "sqlite" || (isset($_POST['HISTORY_DRIVER']) && $_POST['HISTORY_DRIVER'] == "sqlite") ) { ?> selected="selected"<?php } ?>>SQLite3</option>
<?php } ?>
         </select>
      </td>
   </tr>

   <tr id="DIV_HISTORY_HOSTNAME" style="display:show">
      <td>Database host: </td>
      <td><input type="text" name="HISTORY_HOSTNAME" id="HISTORY_HOSTNAME" value="<?php if(isset($a['HISTORY_HOSTNAME'])) { print HISTORY_HOSTNAME; } else if(isset($_POST['HISTORY_HOSTNAME'])) { print $_POST['HISTORY_HOSTNAME']; } else { ?>localhost<?php } ?>" size="30" /></td>
   </tr>

   <tr id="DIV_HISTORY_DATABASE" style="display:show"<?php if(isset($error['history_database'])) { ?> class="error"<?php } ?>>
      <td>History database name: </td>
      <td><input type="text" name="HISTORY_DATABASE" id="HISTORY_DATABASE" value="<?php if(isset($a['HISTORY_DATABASE'])) { print HISTORY_DATABASE; } else if(isset($_POST['HISTORY_DATABASE'])) { print $_POST['HISTORY_DATABASE']; } else { ?>clapf<?php } ?>" size="30" /></td>
      <?php if(isset($error['history_database'])) { ?><td><?php print $error['history_database']; ?></td><?php } ?>
   </tr>

   <tr id="DIV_HISTORY_USERNAME" style="display:show">
      <td>Database user: </td>
      <td><input type="text" name="HISTORY_USERNAME" id="HISTORY_USERNAME" value="<?php if(isset($a['HISTORY_USERNAME'])) { print HISTORY_USERNAME; } else if(isset($_POST['HISTORY_USERNAME'])) { print $_POST['HISTORY_USERNAME']; } else { ?>clapf<?php } ?>" size="30" /></td>
   </tr>

   <tr id="DIV_HISTORY_PASSWORD" style="display:show">
      <td>Database password: </td>
      <td><input type="password" name="HISTORY_PASSWORD" id="HISTORY_PASSWORD" value="<?php if(isset($a['HISTORY_PASSWORD'])) { print HISTORY_PASSWORD; } else if(isset($_POST['HISTORY_PASSWORD'])) { print $_POST['HISTORY_PASSWORD']; } ?>" size="30" /></td>
   </tr>


   <tr>
      <td colspan="2"><hr></td>
   </tr>

   <tr id="DIV_HELPURL" style="display:show">
      <td>Help URL: </td>
      <td><input type="text" name="HELPURL" id="HELPURL" value="<?php if(isset($a['HELPURL'])) { print HELPURL; } else if(isset($_POST['HELPURL'])) { print $_POST['HELPURL']; } ?>" size="30" /></td>
   </tr>


   <tr id="DIV_SESSION_DATABASE" style="display:show"<?php if(isset($error['session_database'])) { ?> class="error"<?php } ?>>
      <td>Session database name: </td>
      <td><input type="text" name="SESSION_DATABASE" id="SESSION_DATABASE" value="<?php if(isset($a['SESSION_DATABASE'])) { print SESSION_DATABASE; } else if(isset($_POST['SESSION_DATABASE'])) { print $_POST['SESSION_DATABASE']; } else { ?>sessions/sessions.sdb<?php } ?>" size="30" /></td>
      <?php if(isset($error['session_database'])) { ?><td><?php print $error['session_database']; ?></td><?php } ?>
   </tr>


   <tr id="DIV_QUARANTINE_DRIVER" style="display:show"<?php if(isset($error['quarantine_database'])) { ?> class="error"<?php } ?>>
      <td>Quarantine database driver: </td>
      <td>mysql <input type="radio" name="QUARANTINE_DRIVER" id="QUARANTINE_DRIVER" value="mysql" <?php if(isset($a['QUARANTINE_DRIVER']) && $a['QUARANTINE_DRIVER'] == "mysql") { ?>checked="checked"<?php } ?> />, sqlite3 <input type="radio" name="QUARANTINE_DRIVER" id="QUARANTINE_DRIVER" value="sqlite" <?php if(isset($a['QUARANTINE_DRIVER']) && $a['QUARANTINE_DRIVER'] == "sqlite") { ?>checked="checked"<?php } ?> /></td>
      <?php if(isset($error['quarantine_database'])) { ?><td><?php print $error['quarantine_database']; ?></td><?php } ?>
   </tr>


   <!-- memcached stuff -->

   <tr>
      <td colspan="2"><hr></td>
   </tr>

   <tr id="DIV_MEMCACHED" style="display:show">
      <td>Memcached support: </td>
      <td>
         <select name="MEMCACHED_ENABLED" id="MEMCACHED_ENABLED" onchange="fix1(); return false;">
            <option value="0"<?php if(MEMCACHED_ENABLED == 0 || (isset($_POST['MEMCACHED_ENABLED']) && $_POST['MEMCACHED_ENABLED'] == 0) ) { ?> selected="selected"<?php } ?>>No</option>
            <option value="1"<?php if(MEMCACHED_ENABLED == 1 || (isset($_POST['MEMCACHED_ENABLED']) && $_POST['MEMCACHED_ENABLED'] == 1) ) { ?> selected="selected"<?php } ?>>Yes</option>
         </select>
      </td>
   </tr>

   <tr id="DIV_MEMCACHED_SERVERS" style="display:none">
      <td>Comma separated list of memcached servers: </td>
      <td><input type="text" name="MEMCACHED_SERVERS" id="MEMCACHED_SERVERS" value="<?php if(isset($a['MEMCACHED_SERVERS'])) { print MEMCACHED_SERVERS; } else if(isset($_POST['MEMCACHED_SERVERS'])) { print $_POST['MEMCACHED_SERVERS']; } else { ?>127.0.0.1:11211<?php } ?>" size="30" /></td>
   </tr>

   <tr>
      <td colspan="2"><hr></td>
   </tr>

   <tr>
      <td colspan="2"><input type="submit" value="Submit"> <input type="reset" value="Cancel"> </td>
   </tr>

</table>

</form>

<?php if($_SERVER['REQUEST_METHOD'] == "POST" || HISTORY_DRIVER){ ?>
<script>fix1(); fix_history();</script>
<?php } ?>

