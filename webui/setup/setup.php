<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="hu" lang="hu">

<head>
   <title>clapf web UI | installer</title>
   <meta http-equiv="content-type" content="text/html; charset=utf-8" />
   <meta http-equiv="Content-Language" content="en" />
   <meta name="keywords" content="clapf, webui, web ui, spam, anti-spam, email, e-mail, mail, unsolicited commercial bulk email, blacklist, software, filtering, service, Bayes, Bayesian" />
   <meta name="description" content="clapf, webui, web ui, spam, anti-spam, antispam" />
   <meta name="rating" content="general" />
   <meta name="robots" content="all" />

   <link rel="stylesheet" type="text/css" href="../view/theme/default/stylesheet/style.css" />
</head>

<body>

<div id="wrap">

  <div id="menu">
  </div> <!-- menu -->

  <div id="content">

  <h3>clapf web ui installer</h3>

<?php

error_reporting(7);

define('CRLF', "\n");

$pdo_drivers = array_flip(PDO::getAvailableDrivers());

if(isset($pdo_drivers['sqlite'])) { define('SQLITE_DRIVER', 1); } else { define('SQLITE_DRIVER', 0); }
if(function_exists("mysql_connect")) { define('MYSQL_DRIVER', 1); } else { define('MYSQL_DRIVER', 0); }


define('WEBUI_DIRECTORY', preg_replace("/\/setup\/setup\.php/", "", $_SERVER['SCRIPT_NAME']) );
define('BASEDIR', preg_replace("/\/$/", "", $_SERVER['DOCUMENT_ROOT']) );

define('CONFIG_FILE', BASEDIR . WEBUI_DIRECTORY . "/config.php");


function check_post_data() {
   $error = array();


   if(!is_readable($_POST['QUEUE_DIRECTORY'])) { $error['queue_directory'] = "cannot read: " . $_POST['QUEUE_DIRECTORY']; }
   if(!is_readable($_POST['STAT_DIRECTORY'])) { $error['stat_directory'] = "cannot read: " . $_POST['STAT_DIRECTORY']; }

   $s = $_POST['SESSION_DATABASE'];
   if(substr($_POST['SESSION_DATABASE'], 0, 1) != "/") { $s = BASEDIR . WEBUI_DIRECTORY . "/" . $_POST['SESSION_DATABASE']; }

   if(!is_writable(dirname($s))) { $error['session_database'] = "cannot write: " . dirname($s); }
   if(!is_writable($s)) { $error['session_database'] = "cannot write: " . $s; }


   if($_POST['HISTORY_DRIVER'] == "sqlite") {
      if(!file_exists($_POST['HISTORY_DATABASE'])) { $error['history_database'] = $_POST['HISTORY_DATABASE'] . " doesn't exist"; }
   }
   if($_POST['HISTORY_DRIVER'] == "mysql") {
      $link = @mysql_connect($_POST['HISTORY_HOSTNAME'], $_POST['HISTORY_USERNAME'], $_POST['HISTORY_PASSWORD']);
      if(!$link || !mysql_select_db($_POST['HISTORY_DATABASE'], $link)) { $error['history_database'] = "cannot connect to history database"; }
      else { mysql_close($link); }
   }


   if($_POST['DB_DRIVER'] == "sqlite") {
      if(!is_writable(dirname($_POST['DB_DATABASE']))) { $error['database'] = "cannot write: " . dirname($_POST['DB_DATABASE']); }
      if(!is_writable($_POST['DB_DATABASE'])) { $error['database'] = "cannot write: " . $_POST['DB_DATABASE']; }
   }
   if($_POST['DB_DRIVER'] == "mysql") {
      $link = @mysql_connect($_POST['DB_HOSTNAME'], $_POST['DB_USERNAME'], $_POST['DB_PASSWORD']);
      if(!$link || !mysql_select_db($_POST['DB_DATABASE'], $link)) { $error['database'] = "cannot connect to database"; }
      else { mysql_close($link); }
   }

   return $error;
}


function write_line($fp, $key = '', $val = '') {
   if($key){
      if(is_numeric($val) && $val != ''){
         fputs($fp, "define('$key', $val);" . CRLF);
      }
      else {
         fputs($fp, "define('$key', '$val');" . CRLF);
      }
   }
   else {
      fputs($fp, CRLF);
   }
}


function write_stuff() {

   $fp = fopen(CONFIG_FILE, "w+");
   if(!$fp){ return 0; }

   fputs($fp, "<?php\n\n");

   fputs($fp, "/* config.php, generated by setup.php, " . date("Y.m.d H:i:s", time()) . " */\n\n\n");

   write_line($fp, "LANG", $_POST['LANG']);
   write_line($fp, "THEME", $_POST['THEME']);
   write_line($fp);


   write_line($fp, "WEBUI_DIRECTORY", WEBUI_DIRECTORY);
   write_line($fp, "QUEUE_DIRECTORY", $_POST['QUEUE_DIRECTORY']);
   write_line($fp, "STAT_DIRECTORY", $_POST['STAT_DIRECTORY']);
   write_line($fp);

   fputs($fp, "define('REMOTE_IMAGE_REPLACEMENT', WEBUI_DIRECTORY . 'view/theme/default/images/remote.gif');" . CRLF);
   write_line($fp);

   write_line($fp, "MAX_CGI_FROM_SUBJ_LEN", 45);
   write_line($fp, "PAGE_LEN", 20);
   write_line($fp);


   write_line($fp, "SMTP_HOST", $_POST['SMTP_HOST']);
   write_line($fp, "SMTP_PORT", $_POST['SMTP_PORT']);
   write_line($fp, "POSTFIX_PORT", 25);
   write_line($fp, "CLAPF_PORT", $_POST['CLAPF_PORT']);
   write_line($fp, "SMTP_DOMAIN", $_POST['SMTP_DOMAIN']);
   write_line($fp, "SMTP_FROMADDR", "no-reply@" . $_POST['SMTP_DOMAIN']);
   write_line($fp, "HAM_TRAIN_ADDRESS", "ham@" . $_POST['SMTP_DOMAIN']);
   write_line($fp, "SPAM_TRAIN_ADDRESS", "spam@" . $_POST['SMTP_DOMAIN']);
   fputs($fp, 'define(\'EOL\', "\n");' . CRLF);
   write_line($fp);
    
   write_line($fp, "CLAPF_HEADER_FIELD", "X-Clapf-spamicity: ");
   write_line($fp);

   write_line($fp, "DEFAULT_POLICY", "default_policy");
   write_line($fp);


   write_line($fp, "DIR_SYSTEM", BASEDIR . WEBUI_DIRECTORY . "/system/");
   write_line($fp, "DIR_MODEL", BASEDIR . WEBUI_DIRECTORY . "/model/");
   write_line($fp, "DIR_DATABASE", BASEDIR . WEBUI_DIRECTORY . "/system/database/");
   write_line($fp, "DIR_IMAGE", BASEDIR . WEBUI_DIRECTORY . "/image/");
   write_line($fp, "DIR_LANGUAGE", BASEDIR . WEBUI_DIRECTORY . "/language/");
   write_line($fp, "DIR_APPLICATION", BASEDIR . WEBUI_DIRECTORY . "/controller/");
   write_line($fp, "DIR_TEMPLATE", BASEDIR . WEBUI_DIRECTORY . "/view/theme/" . $_POST['THEME'] . "/templates/");
   write_line($fp, "DIR_CACHE", BASEDIR . "/cache/");
   write_line($fp, "DIR_REPORT", BASEDIR . "/reports/");
   write_line($fp, "DIR_LOG", BASEDIR . "/log/");
   write_line($fp);

   fputs($fp, "define('QSHAPE_ACTIVE_INCOMING', STAT_DIRECTORY . '/active+incoming');" . CRLF);
   fputs($fp, "define('QSHAPE_ACTIVE_INCOMING_SENDER', STAT_DIRECTORY . '/active+incoming-sender');" . CRLF);
   fputs($fp, "define('QSHAPE_DEFERRED', STAT_DIRECTORY . '/deferred');" . CRLF);
   fputs($fp, "define('QSHAPE_DEFERRED_SENDER', STAT_DIRECTORY . '/deferred-sender');" . CRLF);
   fputs($fp, "define('NUMBER_OF_QUARANTINED_MESSAGES', STAT_DIRECTORY . '/quarantined.messages');" . CRLF);
   fputs($fp, "define('CPUSTAT', STAT_DIRECTORY . '/cpu.stat');" . CRLF);
   fputs($fp, "define('AD_SYNC_STAT', STAT_DIRECTORY . '/adsync.stat');" . CRLF);



   fputs($fp, "define('LOCK_FILE', DIR_LOG . 'lock');" . CRLF);
   fputs($fp, "define('QRUNNER_LOCK_FILE', DIR_LOG . 'qrunner.lock');" . CRLF);

   write_line($fp);

   write_line($fp, "DB_DRIVER", $_POST['DB_DRIVER']);
   write_line($fp, "DB_PREFIX", "");
   write_line($fp, "DB_HOSTNAME", $_POST['DB_HOSTNAME']);
   write_line($fp, "DB_USERNAME", $_POST['DB_USERNAME']);
   write_line($fp, "DB_PASSWORD", $_POST['DB_PASSWORD']);
   write_line($fp, "DB_DATABASE", $_POST['DB_DATABASE']);

   write_line($fp);

   write_line($fp, "TABLE_USER", "user");
   write_line($fp, "TABLE_EMAIL", "t_email");
   write_line($fp, "TABLE_DOMAIN", "t_domain");
   write_line($fp, "TABLE_MISC", "t_misc");
   write_line($fp, "TABLE_WHITELIST", "t_white_list");
   write_line($fp, "TABLE_BLACKLIST", "t_black_list");
   write_line($fp, "TABLE_POLICY", "t_policy");
   write_line($fp, "TABLE_REMOTE", "t_remote");
   write_line($fp, "TABLE_SUMMARY", "summary");
   write_line($fp, "TABLE_COUNTERS", "t_counters");
   write_line($fp, "TABLE_QUARANTINE_GROUP", "t_quarantine_group");

   write_line($fp);

   write_line($fp, "ENABLE_LDAP_IMPORT_FEATURE", 0);
   write_line($fp, "LDAP_IMPORT_CONFIG_FILE", '/path/to/ldap-import-konfig.cfg');
   write_line($fp, "DN_MAX_LEN", 255);
   write_line($fp, "USE_EMAIL_AS_USERNAME", 1);
   write_line($fp, "LDAP_IMPORT_MINIMUM_NUMBER_OF_USERS_TO_HEALTH_OK", 100);

   write_line($fp);

   write_line($fp, "SIZE_X", 430);
   write_line($fp, "SIZE_Y", 250);

   write_line($fp);

   write_line($fp, "QUEUE_DIR_SPLITTING", $_POST['QUEUE_DIR_SPLITTING']);
   write_line($fp);

   write_line($fp, "ENABLE_BLACKLIST", 1);
   write_line($fp, "ENABLE_STATISTICS", 1);
   write_line($fp, "ENABLE_HISTORY", 1);
   write_line($fp, "ENABLE_REMOTE_IMAGES", 0);
   write_line($fp);

   write_line($fp, "SITE_NAME", $_SERVER['SERVER_NAME']);
   write_line($fp, "SITE_URL",  isset($_SERVER['SSL_PROTOCOL']) ? "https://" . $_SERVER['SERVER_NAME'] . WEBUI_DIRECTORY . "/" : "http://" . $_SERVER['SERVER_NAME'] . WEBUI_DIRECTORY . "/");
   write_line($fp);

   write_line($fp, "HELPURL", $_POST['HELPURL']);
   write_line($fp);

   write_line($fp, "DATE_FORMAT", "(Y.m.d.)");
   write_line($fp, "TIMEZONE", 'Europe/Budapest');
   write_line($fp);


   fputs($fp, "define('HISTORY_WORKER_URL', SITE_URL . 'index.php?route=history/worker');" . CRLF);
   fputs($fp, "define('HISTORY_HELPER_URL', SITE_URL . 'index.php?route=history/helper');" . CRLF);

   /* history stuff */

   write_line($fp, "HISTORY_DRIVER", $_POST['HISTORY_DRIVER']);
   write_line($fp, "HISTORY_DATABASE", $_POST['HISTORY_DATABASE']);
   write_line($fp, "HISTORY_HOSTNAME", $_POST['HISTORY_HOSTNAME']);
   write_line($fp, "HISTORY_USERNAME", $_POST['HISTORY_USERNAME']);
   write_line($fp, "HISTORY_PASSWORD", $_POST['HISTORY_PASSWORD']);
   write_line($fp);


   write_line($fp, "HISTORY_REFRESH", 5);
   write_line($fp);

   write_line($fp, "FROM_LENGTH_TO_SHOW", 28);
   write_line($fp);


   fputs($fp, "define('HEALTH_WORKER_URL', SITE_URL . 'index.php?route=health/worker');" . CRLF);
   fputs($fp, "define('HEALTH_REFRESH', 60);" . CRLF);
   fputs($fp, "define('HEALTH_RATIO', 80);" . CRLF);
   write_line($fp);


   write_line($fp, "SESSION_DATABASE", $_POST['SESSION_DATABASE']);
   write_line($fp, "QUARANTINE_DATA", 'sessions/quarantine.sdb');
   write_line($fp, "REMOVE_FROM_QUARANTINE_WILL_UNLINK_FROM_FILESYSTEM", 1);
   write_line($fp);

   fputs($fp, "define('LOG_FILE', DIR_LOG . 'webui.log');" . CRLF);
   write_line($fp, "LOG_DATE_FORMAT", 'd-M-Y H:i:s O');
   write_line($fp);

   write_line($fp, "MIN_PASSWORD_LENGTH", 6);
   write_line($fp);

   write_line($fp, "CGI_INPUT_FIELD_WIDTH", 50);
   write_line($fp, "CGI_INPUT_FIELD_HEIGHT", 7);
   write_line($fp);

   write_line($fp, "PASSWORD_CHANGE_ENABLED", 1);
   write_line($fp);

   write_line($fp, "MEMCACHED_ENABLED", $_POST['MEMCACHED_ENABLED']);
   write_line($fp);

   fputs($fp, '$memcached_servers = array(' . CRLF);

   $x = explode(",", $_POST['MEMCACHED_SERVERS']);
   $i = 0; $ii = count($x);
   foreach ($x as $aaa){
      $_m = explode(":", $aaa);

      $line1  = "      array('" . $_m[0] . "', ";
      $line1 .= isset($_m[1]) ? $_m[1] : "0";
      $line1 .= $i+1 < $ii ? ")," : ")";

      fputs($fp, $line1 . CRLF);

      $i++;
   }
   fputs($fp, '                          );' . CRLF);

   fputs($fp, '$counters = array(\'_c:rcvd\', \'_c:mynetwork\', \'_c:ham\', \'_c:spam\', \'_c:possible_spam\', \'_c:unsure\', \'_c:minefield\', \'_c:virus\', \'_c:zombie\', \'_c:fp\', \'_c:fn\', \'_c:counters_last_update\');' . CRLF);

   write_line($fp);

   fputs($fp, '$health_smtp_servers = array( array("0.0.0.0", POSTFIX_PORT, "postfix-in"), array(SMTP_HOST, CLAPF_PORT, "clapf"), array(SMTP_HOST, SMTP_PORT, "postfix after content-filter") );' . CRLF);

   write_line($fp);

   fputs($fp, '$postgrey_servers = array( );' . CRLF);

   write_line($fp);

   fputs($fp, "?>\n");

   fclose($fp);

   return 1;
}


if($_SERVER['REQUEST_METHOD'] == "POST"){
   $error = check_post_data();

   if(count($error) > 0) {
      require("setup.tpl");
   } else {
      write_stuff();

      print "<p>Written configuration. Remove the write access from " . CONFIG_FILE . ", and you can remove the setup directory</p>\n";
      print "<p>You can <a href='../index.php?route=login/login'>login</a> now.</p>\n";
   }

}
else {
   $stat = stat(CONFIG_FILE);

   if($stat[7] > 30){
      include_once(CONFIG_FILE);
      $a = get_defined_constants();
   }

   if(is_writable(CONFIG_FILE) && is_writable(BASEDIR . "/cache") && is_writable(BASEDIR . "/log")){
      require("setup.tpl");
   }

   if(!is_writable(CONFIG_FILE)) { print "<p>" . CONFIG_FILE . " is not writable.</p>\n"; }
   if(!is_writable(BASEDIR . WEBUI_DIRECTORY . "/cache")) { print "<p>\"cache\" directory is not writable.</p>\n"; }
   if(!is_writable(BASEDIR . WEBUI_DIRECTORY . "/log")) { print "<p>\"log\" directory is not writable.</p>\n"; }


   if($stat[7] > 30) {
      print "You have an existing configuration file. Click on submit to upgrade.\n";
   }
}

?>

   <div id="footer">

   <p>clapf web UI</p>

   </div>


</div> <!-- wrap -->

</body>
</html>
