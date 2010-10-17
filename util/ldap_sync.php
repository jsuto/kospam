<?php

$webuidir = "";

if(isset($_SERVER['argv'][1])) { $webuidir = $_SERVER['argv'][1]; }

require_once($webuidir . "/config.php");
require(DIR_SYSTEM . "/startup.php");
require(DIR_SYSTEM . "/ldap.php");

$cfg = read_konfig(LDAP_IMPORT_CONFIG_FILE);

$verbose = 1;

$loader = new Loader();

$language = new Language();
Registry::set('language', $language);

if(MEMCACHED_ENABLED) {
   $memcache = new Memcache();
   foreach ($memcached_servers as $m){
      $memcache->addServer($m[0], $m[1]);
   }

   Registry::set('memcache', $memcache);
}


$db = new DB(DB_DRIVER, DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PREFIX);
Registry::set('db', $db);

$loader->model('user/user');
$loader->model('user/import');

$import = new ModelUserImport();


foreach ($cfg as $ldap_params) {
   $users = $import->model_user_import->queryRemoteUsers($ldap_params, $ldap_params['domain']);
   $rc = $import->model_user_import->fillRemoteTable($ldap_params, $ldap_params['domain']);

   $import->model_user_import->processUsers($users, $ldap_params, $verbose);
}


?>

Done.
