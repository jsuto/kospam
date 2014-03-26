<?php

$webuidir = "";
$verbose = 0;

if(isset($_SERVER['argv'][1])) { $webuidir = $_SERVER['argv'][1]; }

for($i=2; $i<$_SERVER['argc']; $i++){
   if($_SERVER['argv'][$i] == "verbose") { $verbose = 1; }
}

require_once($webuidir . "/config.php");

require(DIR_SYSTEM . "/startup.php");

$total_notifications = 0;
$failed_notifications = 0;

$loader = new Loader();
Registry::set('load', $loader);

$loader->load->model('user/user');
$loader->load->model('quarantine/database');
$loader->load->model('quarantine/message');
$loader->load->model('mail/mail');

$language = new Language();
Registry::set('language', $language);

Registry::set('admin_user', 1);

$db = new DB(DB_DRIVER, DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PREFIX);
Registry::set('DB_DATABASE', DB_DATABASE);

Registry::set('db', $db);

Registry::set('DB_DRIVER', DB_DRIVER);

$u = new ModelUserUser();
$qd = new ModelQuarantineDatabase();
$mail = new ModelMailMail();


$ldap = new LDAP(LDAP_HOST, LDAP_BIND_DN, LDAP_BIND_PW);
if($ldap->is_bind_ok() == 0) {
   LOGGER(LDAP_BIND_DN . ": failed bind to " . LDAP_HOST);
   exit;
}


/* get a lock, to prevent a paralel running */

$fp = fopen(LOCK_FILE, "r");
if(!$fp) { die("cannot open: " . LOCK_FILE . "\n"); }
if(!flock($fp, LOCK_EX | LOCK_NB)) { fclose($fp); die("cannot get a lock on " . LOCK_FILE . "\n"); }


$users = $u->getUsers();

if($verbose >= 1) { print "queried users: " . count($users) . "\n"; }

extract($language->data);

if(QUARANTINE_DRIVER == "mysql") {
   Registry::set('Q', $db);
}
if(QUARANTINE_DRIVER == "sqlite") {
   $Q = new DB(QUARANTINE_DRIVER, "", "", "", QUARANTINE_DATABASE, "");
   Registry::set('Q', $Q);
}


Registry::set('admin_user', 0);

foreach ($users as $user) {

  if(strlen($user["dn"]) < 5 || !strstr($user["dn"], "OU") ) { continue; }

   $basedn = strstr($user['dn'], "OU");

   $query = $ldap->query($basedn, "mail=" . $user['email'], array("objectclass") );
   if(!isset($query->row["objectclass"])) { continue; }

   $send_report = 0;

   for($i=0; $i<$query->row["objectclass"]["count"]; $i++) {
      if($query->row["objectclass"][$i] == "user") { $send_report = 1; }
   }

   if($send_report == 0) {
      continue;
   }


   /* check if this account is a list address, and if so, then skip it */

   $q = $db->query("SELECT COUNT(*) AS num FROM " . TABLE_QUARANTINE_GROUP . " WHERE gid=" . (int)$user['uid']);
   if(isset($q->row['num']) && $q->row['num'] > 0) { continue; }

   list ($n, $total_size, $messages) = $qd->getMessages(array($user['uid']), 0, 0, '', '', '', 'SPAM', 'ts', 1);

   if($n > 0) {

      $total_notifications++;

      $msg = "From: " . SMTP_FROMADDR . EOL;
      $msg .= "To: " . $user['email'] . EOL;
      $msg .= "Subject: =?UTF-8?Q?" . preg_replace("/\n/", "", my_qp_encode($text_daily_quarantine_report)) . "?=" . EOL;
      $msg .= "MIME-Version: 1.0" . EOL;
      $msg .= "Content-Type: text/html; charset=\"utf-8\"" . EOL;
      $msg .= EOL . EOL;

      ob_start();

      include($webuidir . "/language/" . LANG . "/quarantine-daily-digest.tpl");

      $msg .= ob_get_contents();

      ob_end_clean();

      $x = $mail->SendSmtpEmail(LOCALHOST, POSTFIX_PORT_AFTER_CONTENT_FILTER, SMTP_DOMAIN, SMTP_FROMADDR, $user['email'], $msg);

      if($x == 0) { $failed_notifications++; }
   }

}


print date(LOG_DATE_FORMAT) . ", $text_total/$text_failed: $total_notifications/$failed_notifications\n";


/* release lock */

if($fp) {
   flock($fp, LOCK_UN);
   fclose($fp);
}


?>
