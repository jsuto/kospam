<?php

function extract_mysql_dsn($dsn){
   $host = $u = $p = $db = "";

   $aa = substr($dsn, strlen("mysql://"), strlen($dsn));

   list($a1, $a2) = explode("@", $aa);

   list($u, $p) = explode(":", $a1);
   list($host, $db) = explode("/", $a2);

   return array ($host, $u, $p, $db);
}


function check_user_from_password_file(){
   global $password_file;
   $ok = 0;

   $fp = fopen($password_file, "r");
   if(!$fp) return $ok;

   while($l = fgets($fp, 512)){
      $l = rtrim($l);
      list($u, $p) = explode(":", $l);

      if($u == $_SERVER['PHP_AUTH_USER']){
         $pass = crypt($_SERVER['PHP_AUTH_PW'], $p);
         if($pass == $p){ $ok = 1; break; }
      }
   }

   fclose($fp);

   return $ok;
}


function check_user_from_sql_table(){
   global $user_table, $err_sql_error;
   $p = "";
   $ok = 0;

   $u = $_SERVER['PHP_AUTH_USER'];

   webui_connect() or nice_error($err_connect_db);

   $stmt = "SELECT password FROM $user_table WHERE username='$u'";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($p) = mysql_fetch_row($r);
   mysql_free_result($r);

   if($p == "") return $ok;

   $pass = crypt($_SERVER['PHP_AUTH_PW'], $p);
   if($pass == $p) $ok = 1;

   return $ok;
}


function get_authenticated_username(){
   global $password_file;

   if(isset($_SESSION['username'])) return $_SESSION['username'];

   if(isset($_SERVER['PHP_AUTH_USER']) && isset($_SERVER['PHP_AUTH_PW']) && !empty($_SESSION['auth'])){

      /* check whether the given username + password is ok */

      if($password_file){
         if(check_user_from_password_file() == 1){
            $_SESSION['username'] = $_SERVER['PHP_AUTH_USER'];
            return $_SERVER['PHP_AUTH_USER'];
         }
      }
      else {
         if(check_user_from_sql_table() == 1){
            $_SESSION['username'] = $_SERVER['PHP_AUTH_USER'];
            return $_SERVER['PHP_AUTH_USER'];
         }
      }
   }
   else return "";
}


function show_auth_popup(){
   global $err_not_authenticated;

   header('WWW-Authenticate: Basic Realm="webui - login please"');
   header('HTTP/1.0 401 Unauthorized');
   $_SESSION['auth'] = true;
   nice_error($err_not_authenticated);
}


function logout(){
   $_SESSION['auth'] = null;
   $_SESSION['username'] = "";
   session_destroy();
}


?>
