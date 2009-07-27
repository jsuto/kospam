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


if(Registry::get('username')) {

   if(isset($request->get['route'])){
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
