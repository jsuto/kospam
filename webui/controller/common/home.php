<?php


class ControllerCommonHome extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "common/home.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('user/auth');



      $this->document->title = $this->data['text_home'];

      if(isset($this->request->get['pagelen']) && is_numeric($this->request->get['pagelen']) && $this->request->get['pagelen'] > 5) {
          Header("Set-Cookie: pagelen=" . $this->request->get['pagelen'] . "; path=/");
      }


      if($this->request->server['REQUEST_METHOD'] == 'POST' && $this->validate() == true) {

         if($this->model_user_auth->QQchangePassword(Registry::get('username'), $this->request->post['password']) == 1) {
            $this->data['x'] = $this->data['text_password_changed'];
         }
         else {
            $this->data['x'] = $this->data['text_failed_to_change_password'];
         }
      }


      $this->render();
   }


   private function validate() {

      if(!isset($this->request->post['password']) || !isset($this->request->post['password2']) ) {
         $this->error['password'] = $this->data['text_missing_password'];
      }

      if(strlen(@$this->request->post['password']) < 8 || strlen(@$this->request->post['password2']) < 8) {
         $this->error['password'] = $this->data['text_invalid_password'];
      }

      if($this->request->post['password'] != $this->request->post['password2']) {
         $this->error['password'] = $this->data['text_password_mismatch'];
      }


      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }



}

?>
