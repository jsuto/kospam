<?php


class ControllerLoginLogout extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "login/logout.tpl";
      $this->layout = "common/layout";

      logout();

      $request = Registry::get('request');
      $db = Registry::get('db');

      if(DB_DRIVER == "ldap")
         $this->load->model('user/ldap/auth');
      else
         $this->load->model('user/sql/auth');

      $this->document->title = $this->data['text_logout'];


      $this->render();
   }


}

?>
