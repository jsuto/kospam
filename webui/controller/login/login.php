<?php


class ControllerLoginLogin extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "login/login.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('user/auth');

      $this->document->title = $this->data['text_login'];

      if($this->request->server['REQUEST_METHOD'] == 'POST' && $this->validate() == true) {
         if($this->model_user_auth->checkLogin($this->request->post['username'], $this->request->post['password']) == 1) {
            header("Location: " . SITE_URL . "index.php?route=quarantine/quarantine");
            exit;
         }

         $this->data['x'] = $this->data['text_invalid_email_or_password'];

      }


      $this->render();
   }


   private function validate() {

      if(strlen($this->request->post['username']) < 2){
         $this->error['username'] = $this->data['text_invalid_username'];
      }


      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }


}

?>
