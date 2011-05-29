<?php

$webuidir = "";
$verbose = 0;

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

$Q = new DB("sqlite", "", "", "", "$webuidir/" . QUARANTINE_DATA, "");
Registry::set('Q', $Q);

$st = stat("$webuidir/" . QUARANTINE_DATA);
if(isset($st['size']) && $st['size'] < 1024) {
   $qd->model_quarantine_database->CreateDatabase();
   chmod("$webuidir/" . QUARANTINE_DATA, 0660);
}


foreach ($users as $user) {

   $my_q_dir = get_per_user_queue_dir($user['domain'], $user['username'], $user['uid']);

   if(file_exists($my_q_dir)) {
      $qd->PopulateDatabase($my_q_dir, $user['uid']);
   }
}

$Q->query("vacuum");


/* release lock */

flock($fp, LOCK_UN);
fclose($fp);


?>
