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


/* get a lock, to prevent a paralel running */

$fp = fopen(LOCK_FILE, "r");
if(!$fp) { die("cannot open: " . LOCK_FILE . "\n"); }
if(!flock($fp, LOCK_EX | LOCK_NB)) { fclose($fp); die("cannot get a lock on " . LOCK_FILE . "\n"); }


$users = $u->getUsers();

if($verbose >= 1) { print "queried users: " . count($users) . "\n"; }

extract($language->data);


foreach ($users as $user) {
   $domain = $u->getDomainsByUid($user['uid']);
   $my_q_dir = get_per_user_queue_dir($domain[0], $user['username'], $user['uid']);

   if(file_exists($my_q_dir)) {

      $total_notifications++;

      $Q = new DB("sqlite", "", "", "", $my_q_dir . "/" . QUARANTINE_DATA, "");
      Registry::set('Q', $Q);

      /* create schema if it's a 0 byte length file */

      $st = stat($my_q_dir . "/" . QUARANTINE_DATA);
      if(isset($st['size']) && $st['size'] < 1024) {
         $qd->CreateDatabase();
      }


      /* populate quarantine files to database if we are on the 1st page */

      $qd->PopulateDatabase($my_q_dir);

      list ($n, $total_size, $messages) = $qd->getMessages($my_q_dir, $user['username'], 0, 0, '', '', 'SPAM', 'ts', 1);

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

      $x = $mail->SendSmtpEmail(SMTP_HOST, SMTP_PORT, SMTP_DOMAIN, SMTP_FROMADDR, $user['email'], $msg);
      if($x == 0) { $failed_notifications++; }


   }
}


if($verbose >= 1) { print "total/failed notifications: $total_notifications/$failed_notifications\n"; }


/* release lock */

flock($fp, LOCK_UN);
fclose($fp);


?>
