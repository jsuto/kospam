<?php

function go_to_setup() {
   Header("Location: setup/setup.php");
   exit;
}

$stat = stat("config.php") or go_to_setup();
if($stat[7] < 15){ go_to_setup(); }


require_once("config.php");

require(DIR_SYSTEM . "/startup.php");


$request = new Request();
Registry::set("request", $request);


session_start();


Registry::set('document', new Document());


$loader = new Loader();
Registry::set('load', $loader);


$language = new Language();
Registry::set('language', $language);


/* check if user has authenticated himself. If not, we send him to login */

Registry::set('username', getAuthenticatedUsername());
Registry::set('admin_user', isAdminUser());
Registry::set('domain_admin', isDomainAdmin());
Registry::set('readonly_admin', isReadonlyAdmin());


$db = new DB(DB_DRIVER, DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PREFIX);
Registry::set('DB_DATABASE', DB_DATABASE);

Registry::set('db', $db);

Registry::set('DB_DRIVER', DB_DRIVER);

Registry::set('HISTORY_DATABASE', HISTORY_DATABASE);
Registry::set('HISTORY_DRIVER', HISTORY_DRIVER);


if(MEMCACHED_ENABLED) {
   $memcache = new Memcache();
   foreach ($memcached_servers as $m){
      $memcache->addServer($m[0], $m[1]);
   }

   Registry::set('memcache', $memcache);
}

Registry::set('counters', $counters);

Registry::set('health_smtp_servers', $health_smtp_servers);
Registry::set('postgrey_servers', $postgrey_servers);
Registry::set('partitions_to_monitor', $partitions_to_monitor);


if(Registry::get('username')) {

   if(isset($request->get['route'])){

      if($request->get['route'] == "history/worker" || $request->get['route'] == "history/view" || $request->get['route'] == "history/helper" || strstr($request->get['route'], "stat/") || strstr($request->get['route'], "health/") ) {
         $db_history = new DB(HISTORY_DRIVER, DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, HISTORY_DATABASE, DB_PREFIX);
         Registry::set('db_history', $db_history);
      }

      if(strstr($request->get['route'], "quarantine/")) {
         if(QUARANTINE_DRIVER == "mysql") {
            Registry::set('Q', $db);
         }
         if(QUARANTINE_DRIVER == "sqlite") {
            $Q = new DB(QUARANTINE_DRIVER, "", "", "", QUARANTINE_DATABASE, "");
            Registry::set('Q', $Q);
         }

      }

      $action = new Router($request->get['route']);
   }
   else {
      $action = new Router('common/home');
   }
}
else {
   $action = new Router('login/login');
}


$controller = new Front();
$controller->dispatch($action, new Router('common/not_found'));


?>
