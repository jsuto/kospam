<?php

$webuidir = "";
$verbose = 0;

$time_start = microtime(true);

if(isset($_SERVER['argv'][1])) { $webuidir = $_SERVER['argv'][1]; }

for($i=2; $i<$_SERVER['argc']; $i++){
   if($_SERVER['argv'][$i] == "verbose") { $verbose = 1; }
}

require_once($webuidir . "/config.php");

require(DIR_SYSTEM . "/startup.php");

$loader = new Loader();
Registry::set('load', $loader);

$loader->load->model('user/user');
$loader->load->model('quarantine/database');
$loader->load->model('quarantine/message');

$language = new Language();
Registry::set('language', $language);

Registry::set('admin_user', 1);

$db = new DB(DB_DRIVER, DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PREFIX);
Registry::set('DB_DATABASE', DB_DATABASE);

Registry::set('db', $db);

Registry::set('DB_DRIVER', DB_DRIVER);

$u = new ModelUserUser();
$qd = new ModelQuarantineDatabase();


/* get a lock, to prevent a paralel running */

$fp = fopen(QRUNNER_LOCK_FILE, "r");
if(!$fp) { die("cannot open: " . QRUNNER_LOCK_FILE . "\n"); }
if(!flock($fp, LOCK_EX | LOCK_NB)) { fclose($fp); die("cannot get a lock on " . QRUNNER_LOCK_FILE . "\n"); }


$users = $u->getUsers();

if($verbose >= 1) { print "queried users: " . count($users) . "\n"; }


if(QUARANTINE_DRIVER == "mysql") {
   Registry::set('Q', $db);
   $qd->CreateDatabase();
}

if(QUARANTINE_DRIVER == "sqlite") {
   $Q = new DB(QUARANTINE_DRIVER, "", "", "", QUARANTINE_DATABASE, "");
   Registry::set('Q', $Q);

   $st = stat(QUARANTINE_DATABASE);
   if(isset($st['size']) && $st['size'] < 1024) {
      $qd->CreateDatabase();
      @chmod(QUARANTINE_DATABASE, 0660);
   }
}


$total = 0;

foreach ($users as $user) {

   $my_q_dir = get_per_user_queue_dir($user['domain'], $user['username'], $user['uid']);

   $group_q_dirs = $u->get_quarantine_directories($user['uid']);

   if(file_exists($my_q_dir)) {
      $total += $qd->PopulateDatabase($my_q_dir, $user['uid'], $group_q_dirs);
   }
}

if(QUARANTINE_DRIVER == "sqlite") { $Q->query("vacuum"); }


/* release lock */

flock($fp, LOCK_UN);
fclose($fp);


$time_end = microtime(true);

$exec_time = $time_end - $time_start;

openlog("qrunner", LOG_PID, LOG_MAIL);
syslog(LOG_INFO, sprintf("processed %d messages in %.2f sec", $total, $exec_time));

?>
