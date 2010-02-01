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

      if(isset($this->request->get['pagelen']) && is_numeric($this->request->get['pagelen']) && $this->request->get['pagelen'] >= 10 && $this->request->get['pagelen'] <= 50) {
         $_SESSION['pagelen'] = $this->request->get['pagelen'];
      }


      if(isset($this->request->get['lang']) && strlen($this->request->get['lang']) == 2 && file_exists(DIR_LANGUAGE . $this->request->get['lang']) ) {
         $_SESSION['lang'] = $this->request->get['lang'];

         Header("Location: index.php?route=common/home");
         return;
      }


      if($this->request->server['REQUEST_METHOD'] == 'POST' && $this->validate() == true) {

         if($this->model_user_auth->changePassword(Registry::get('username'), $this->request->post['password']) == 1) {
            $this->data['x'] = $this->data['text_password_changed'];
         }
         else {
            $this->data['x'] = $this->data['text_failed_to_change_password'];
         }
      }


      $this->data['page_len'] = getPageLength();

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
