<?php


class ControllerUserSettings extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "user/settings.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('user/auth');
      $this->load->model('user/prefs');


      $this->document->title = $this->data['text_home'];


      if(isset($this->request->post['pagelen']) && isset($this->request->post['lang']) ) {
         $db_session = new DB("sqlite", "", "", "", SESSION_DATABASE, "");

         $this->model_user_prefs->setUserPrefs($db_session, Registry::get('username'), $this->request->post);

         Header("Location: index.php?route=user/settings");
         return;
      }


      if($this->request->server['REQUEST_METHOD'] == 'POST' && PASSWORD_CHANGE_ENABLED == 1 && $this->validate() == true) {

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

      if(strlen(@$this->request->post['password']) < MIN_PASSWORD_LENGTH || strlen(@$this->request->post['password2']) < MIN_PASSWORD_LENGTH) {
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
