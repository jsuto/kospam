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

define('CONFIG_FILE', "config.php");
define('CRLF', "\n");


define('WEBUI_DIRECTORY', preg_replace("/\/setup\/setup\.php/", "", $_SERVER['SCRIPT_NAME']) );
define('BASEDIR', preg_replace("/\/$/", "", $_SERVER['DOCUMENT_ROOT']) );


function write_line($fp, $key = '', $val = '') {
   if($key){
      if(is_numeric($val)){
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

   $fp = fopen("../" . CONFIG_FILE, "w+");
   if(!$fp){ return 0; }

   fputs($fp, "<?php\n\n");

   fputs($fp, "/* config.php, generated by setup.php, " . date("Y.m.d H:i:s", time()) . " */\n\n\n");

   write_line($fp, "LANG", $_POST['LANG']);
   write_line($fp, "THEME", $_POST['THEME']);
   write_line($fp);

   write_line($fp, "WEBUI_DIRECTORY", WEBUI_DIRECTORY);
   write_line($fp, "QUEUE_DIRECTORY", $_POST['QUEUE_DIRECTORY']);
   write_line($fp);

   write_line($fp, "MAX_CGI_FROM_SUBJ_LEN", 45);
   write_line($fp, "PAGE_LEN", 20);
   write_line($fp);


   write_line($fp, "SMTP_HOST", $_POST['SMTP_HOST']);
   write_line($fp, "SMTP_PORT", $_POST['SMTP_PORT']);
   write_line($fp, "CLAPF_PORT", $_POST['CLAPF_PORT']);
   write_line($fp, "SMTP_DOMAIN", $_POST['SMTP_DOMAIN']);
   write_line($fp, "SMTP_FROMADDR", "no-reply@" . $_POST['SMTP_DOMAIN']);
   write_line($fp, "HAM_TRAIN_ADDRESS", "ham@" . $_POST['SMTP_DOMAIN']);
   write_line($fp);
    
   write_line($fp, "CLAPF_HEADER_FIELD", "X-Clapf-spamicity: ");
   write_line($fp);

   write_line($fp, "DEFAULT_POLICY", "default_policy");
   write_line($fp);


   write_line($fp, "DIR_SYSTEM", BASEDIR . WEBUI_DIRECTORY . "/system/");

   if($_POST['DB_DRIVER'] == "ldap") {
      write_line($fp, "DIR_MODEL", BASEDIR . WEBUI_DIRECTORY . "/model-ldap/");
   }
   else {
      write_line($fp, "DIR_MODEL", BASEDIR . WEBUI_DIRECTORY . "/model/");
   }

   write_line($fp, "DIR_DATABASE", BASEDIR . WEBUI_DIRECTORY . "/system/database/");
   write_line($fp, "DIR_IMAGE", BASEDIR . WEBUI_DIRECTORY . "/image/");
   write_line($fp, "DIR_LANGUAGE", BASEDIR . WEBUI_DIRECTORY . "/language/");
   write_line($fp, "DIR_APPLICATION", BASEDIR . WEBUI_DIRECTORY . "/controller/");
   write_line($fp, "DIR_TEMPLATE", BASEDIR . WEBUI_DIRECTORY . "/view/theme/" . $_POST['THEME'] . "/templates/");
   write_line($fp, "DIR_CACHE", BASEDIR . "/cache/");

   write_line($fp);


   write_line($fp, "DB_DRIVER", $_POST['DB_DRIVER']);

   if($_POST['DB_DRIVER'] == "ldap") {
      write_line($fp, "LDAP_HOST", $_POST['LDAP_HOST']);
      write_line($fp, "LDAP_BINDDN", $_POST['LDAP_BINDDN']);
      write_line($fp, "LDAP_BINDPW", $_POST['LDAP_BINDPW']);
      write_line($fp, "LDAP_USER_BASEDN", $_POST['LDAP_USER_BASEDN']);
      write_line($fp, "LDAP_POLICY_BASEDN", $_POST['LDAP_POLICY_BASEDN']);
      write_line($fp, "LDAP_DOMAIN_BASEDN", $_POST['LDAP_DOMAIN_BASEDN']);
      write_line($fp);
   }
   else {
      write_line($fp, "DB_HOSTNAME", $_POST['DB_HOSTNAME']);
      write_line($fp, "DB_USERNAME", $_POST['DB_USERNAME']);
      write_line($fp, "DB_PASSWORD", $_POST['DB_PASSWORD']);
      write_line($fp, "DB_DATABASE", $_POST['DB_DATABASE']);
      write_line($fp, "DB_PREFIX", "");
      write_line($fp);

      write_line($fp, "TABLE_USER", "user");
      write_line($fp, "TABLE_EMAIL", "t_email");
      write_line($fp, "TABLE_DOMAIN", "t_domain");
      write_line($fp, "TABLE_MISC", "t_misc");
      write_line($fp, "TABLE_WHITELIST", "t_white_list");
      write_line($fp, "TABLE_BLACKLIST", "t_black_list");
      write_line($fp, "TABLE_POLICY", "t_policy");
      write_line($fp, "TABLE_STAT", "t_stat");
      write_line($fp, "TABLE_REMOTE", "t_remote");
   }

   write_line($fp);

   write_line($fp, "QUEUE_DIR_SPLITTING", $_POST['QUEUE_DIR_SPLITTING']);
   write_line($fp);

   write_line($fp, "ENTERPRISE_VERSION", 0);
   write_line($fp);

   write_line($fp, "SITE_URL", "http://" . $_SERVER['SERVER_NAME'] . WEBUI_DIRECTORY . "/");
   write_line($fp);

   write_line($fp, "DATE_FORMAT", "(Y.m.d.)");
   write_line($fp, "TIMEZONE", 'Europe/Budapest');
   write_line($fp);


   fputs($fp, "define('HISTORY_WORKER_URL', SITE_URL . 'index.php?route=history/worker');" . CRLF);
   fputs($fp, "define('HISTORY_HELPER_URL', SITE_URL . 'index.php?route=history/helper');" . CRLF);
   write_line($fp, "HISTORY_DATA", '/var/lib/clapf/data/log.sdb');
   write_line($fp, "HISTORY_REFRESH", 5);
   write_line($fp, "HISTORY_ENTRIES_PER_PAGE", 15);



   write_line($fp, "CGI_INPUT_FIELD_WIDTH", 50);
   write_line($fp, "CGI_INPUT_FIELD_HEIGHT", 7);
   write_line($fp);

   fputs($fp, "?>\n");

   fclose($fp);

   return 1;
}


if($_SERVER['REQUEST_METHOD'] == "POST"){
   write_stuff();

   print "<p>Written configuration. Remove the write access from " . CONFIG_FILE . ", and you can remove the setup directory</p>\n";
   print "<p>You can <a href='../index.php?route=login/login'>login</a> now.</p>\n";
}
else {
   $stat = stat("../" . CONFIG_FILE);

   if($stat[7] < 30){
      if(is_writable("../" . CONFIG_FILE)){
         require("setup.tpl");
      }
      else {
         print CONFIG_FILE . " is not writable.\n";
      }
   }
   else {
      print "You have an existing configuration file. Exiting.\n";
   }
}

?>

   <div id="footer">

   <p>clapf web UI</p>

   </div>


</div> <!-- wrap -->

</body>
</html>
