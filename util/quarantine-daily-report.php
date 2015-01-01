<?php

$webuidir = "";
$verbose = 0;
$dry_run = 0;


$opts = 'dhv';
$lopts = array(
    'webui:',
    'dry-run',
    'verbose'
    );
    
if($options = getopt($opts, $lopts)) {

    if(isset($options['webui'])) { 
        $webuidir = $options['webui'];
    } else {
        echo("\nError: must provide path to WebUI directory\n\n");
        display_help();
        exit;
    }

    if(isset($options['dry-run']) || isset($options['d'])) {
        $dry_run = 1;
    }

    if(isset($options['h'])) { 
        display_help();
        exit;
    }

    if(isset($options['verbose']) || isset($options['v'])) {
        $verbose = 1;
    }

}
else {
    display_help();
    exit;   
}

ini_set("session.save_path", "/tmp");

require_once($webuidir . "/config.php");
require(DIR_SYSTEM . "/startup.php");
date_default_timezone_set(TIMEZONE);

$total_notifications = 0;
$failed_notifications = 0;

$page_len = 1000;
$page = 0;

$loader = new Loader();
Registry::set('load', $loader);

$loader->load->model('user/user');
$loader->load->model('search/search');
$loader->load->model('search/message');
$loader->load->model('mail/mail');

$language = new Language();
Registry::set('language', $language);

extract($language->data);

Registry::set('admin_user', 1);

$db = new DB(DB_DRIVER, DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PREFIX);
Registry::set('DB_DATABASE', DB_DATABASE);

Registry::set('db', $db);

Registry::set('DB_DRIVER', DB_DRIVER);

$sphx = new DB(SPHINX_DRIVER, SPHINX_HOSTNAME, "", "", SPHINX_DATABASE, "");
Registry::set('sphx', $sphx);

$title = $text_daily_quarantine_report;


$u = new ModelUserUser();
$mail = new ModelMailMail();
$search = new ModelSearchSearch();


if(ENABLE_LDAP_AUTH == 1) {
   $ldap = new LDAP(LDAP_HOST, LDAP_HELPER_DN, LDAP_HELPER_PASSWORD);
   if($ldap->is_bind_ok() == 0) {
      LOGGER(LDAP_HELPER_DN . ": failed bind to " . LDAP_HOST);
      exit;
   }
}


$users = $u->get_users('', $page, $page_len);

if($verbose >= 1) { print "queried users: " . count($users) . "\n"; }

Registry::set('admin_user', 0);

foreach ($users as $user) {

   if(ENABLE_LDAP_AUTH == 1) {
      if(strlen($user["dn"]) < 5 || !strstr($user["dn"], "OU") ) { continue; }

      $basedn = strstr($user['dn'], "OU");

      $ldap_mail_attr = LDAP_MAIL_ATTR;
      if($ldap_mail_attr == 'proxyAddresses') { $ldap_mail_attr = "proxyaddresses=smtp:"; }

      $query = $ldap->query($user['dn'], "(|(mail=" . $user['email'] . ")($ldap_mail_attr" . $user['email'] . "))", array("objectclass", "mail", "mailalternateaddress", "proxyaddresses", "zimbraMailForwardingAddress") );

      if(!isset($query->row["objectclass"])) { continue; }

      $send_report = 0;

      for($i=0; $i<$query->row["objectclass"]["count"]; $i++) {
         if($query->row["objectclass"][$i] == "user") { $send_report = 1; }
      }

      if($send_report == 0) {
         continue;
      }

      /*
       * TODO:
       *
       * check if this account is a list address, and if so, then skip it
       */


   }


   $session->set("username", $user['username']);
   $session->set("uid", $user['uid']);
   $session->set("admin_user", 0);
   $session->set("email", $user['email']);
   $session->set("domain", $user['domain']);
   $session->set("emails", array());
   $session->set("pagelen", MAX_SEARCH_HITS);

   $a = $u->get_users_all_email_addresses($user['uid']);

   $session->set("emails", $a);

   $data = array();

   $data['spam'] = 1;
   $data['sort'] = 'ts';
   $data['order'] = 0;
   $data['match'] = array();
   $data['date1'] = '';
   $data['date2'] = '';

   list ($n, $total_found, $all_ids, $messages) = $search->search_messages($data, 0);

   if($dry_run == 0) {
      if($n > 0) {
         $msg = "From: " . SMTP_FROMADDR . EOL;
         $msg .= "To: " . ADMIN_EMAIL . EOL;
         $msg .= "Subject: =?UTF-8?Q?" . preg_replace("/\n/", "", my_qp_encode($title)) . "?=" . EOL;
         $msg .= "Message-ID: <" . generate_random_string(25) . '@' . SITE_NAME . ">" . EOL;
         $msg .= "MIME-Version: 1.0" . EOL;
         $msg .= "Content-Type: text/html; charset=\"utf-8\"" . EOL;
         $msg .= EOL . EOL;

         ob_start();
         include($webuidir . "/view/theme/default/templates/search/auto.tpl");
         $msg .= ob_get_contents();

         ob_end_clean();

         $x = $mail->send_smtp_email(SMARTHOST, SMARTHOST_PORT, SMTP_DOMAIN, SMTP_FROMADDR, array($session->get("email")), $msg);
      }
   }
   else {
      print $user['username'] . ", count=$n ($all_ids)". EOL;
   }

}


function display_help() {
    echo("\nUsage: " . basename(__FILE__) . " --webui [PATH] [OPTIONS...]\n\n");
    echo("\t--webui=\"[REQUIRED: path to the clapf webui directory]\"\n\n");
    echo("options:\n");
    echo("\t-h Prints this help screen and exits\n");
    echo("\n");
}

?>
