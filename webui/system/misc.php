<?php


function getAuthenticatedUsername() {

   if(isset($_SESSION['username'])){ return $_SESSION['username']; }

   return "";
}


function isAdminUser() {
   if(isset($_SESSION['admin_user']) && $_SESSION['admin_user'] == 1){ return 1; }

   return 0;
}


function logout() {
   $_SESSION['username'] = "";
   $_SESSION['admin_user'] = 0;

   session_destroy();
}


function isBinary($num = '') {
   if($num == 0 || $num == 1){ return 1; }

   return 0; 
}


function getPageLength() {
   $page_len = PAGE_LEN;
   
   if(isset($_COOKIE['pagelen']) && is_numeric($_COOKIE['pagelen']) && $_COOKIE['pagelen'] >= 10 && $_COOKIE['pagelen'] <= 50) {
      $page_len = $_COOKIE['pagelen'];
   }

   return $page_len;
}


function checkemail($email) {
   if($email == ""){
      return 0;
   }

   if (eregi('^[_a-z0-9-]+(\.[_a-z0-9-]+)*@[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,4})$', $email)) {
      return 1;
   }

   return 0;
}


function first_n_characters($what, $n){
   $x = "";
   $len = 0;

   $a = explode(" ", $what);
   while(list($k, $v) = each($a)){
      $x .= "$v "; $len += strlen($v) + 1;
      if($len >= $n){ return $x . "..."; }
   }

   return $x . "...";
}


function get_per_user_queue_dir($username = '', $uid = 0){

   if(QUEUE_DIR_SPLITTING == 1) {
      if(!is_numeric($uid) || $uid <= 0){ return ""; }

      $h = $uid;

      $i = $h % 10000;
      if($i > 0) {
         $plus1 = 1;
      }
      else {
         $plus1 = 0;
      }

      $i = $h % 100;
      if($i > 0){
         $plus1b = 1;
      }
      else {
         $plus1b = 0;
      }

      return QUEUE_DIRECTORY . "/" . 10000 * (floor($h / 10000) + $plus1) . "/" . 100 * (floor($h / 100) + $plus1b) . "/" . $uid;

   }
   else {
      return QUEUE_DIRECTORY . "/" . substr($username, 0, 1) . "/" . $username;
   }

}


?>
