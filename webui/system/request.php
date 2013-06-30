<?php

class Request {
   public $get = array();
   public $post = array();
   public $cookie = array();
   public $files = array();
   public $server = array();


   public function __construct() {
       $this->get    =& $this->clean($_GET);
       $this->post   =& $this->clean($_POST);
       $this->cookie =& $this->clean($_COOKIE);
       $this->files  =& $this->clean($_FILES);
       $this->server =& $this->clean($_SERVER);
   }


   public function clean($data) {
      if (is_array($data)) {
         foreach ($data as $key => $value) {
            $data[$key] =& $this->clean($value);
         }
      } else {
          $data = htmlspecialchars($data, ENT_QUOTES, 'UTF-8');
      }

      return $data;
   }


}


class Session {

   public function __construct() {
      session_start();
   }


   public function get($s = '') {
      if($s && isset($_SESSION[$s])) { return $_SESSION[$s]; }

      return '';
   }


   public function set($k = '', $v = '') {

      if($k) { $_SESSION[$k] = $v; }

   }

}


?>
