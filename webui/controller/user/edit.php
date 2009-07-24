<?php


class ControllerUserEdit extends Controller {
   private $error = array();

   public function index(){
      $this->data['uid'] = 0;
      $this->data['email'] = "";

      $this->id = "content";
      $this->template = "user/edit.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');
      $language = Registry::get('language');

      $this->load->model('user/user');
      $this->load->model('policy/policy');

      $this->document->title = $language->get('text_user_management');


      if(isset($this->request->get['uid']) && is_numeric($this->request->get['uid']) && $this->request->get['uid'] > 0) {
         $this->data['uid'] = $this->request->get['uid'];
      }

      if(isset($this->request->get['email']) && checkemail($this->request->get['email']) == 1) {
         $this->data['email'] = $this->request->get['email'];
      }

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         if($this->request->server['REQUEST_METHOD'] == 'POST') {
            if($this->validate() == true){

               $ret = $this->model_user_user->updateUser($this->request->post);

               if($ret == 1){
                  $this->data['x'] = $this->data['text_successfully_modified'];
               } else {
                  $this->template = "common/error.tpl";
                  $this->data['errorstring'] = $this->data['text_failed_to_modify'];
               }

               $this->model_user_user->setWhitelist($this->request->post['username'], $this->request->post['whitelist']);
               $this->model_user_user->setBlacklist($this->request->post['username'], $this->request->post['blacklist']);

            }
            else {
               $this->template = "common/error.tpl";
               $this->data['errorstring'] = array_pop($this->error);
            }
         }
         else {
            $this->data['user'] = $this->model_user_user->getUserByUid($this->data['uid']);
            $this->data['user']['whitelist'] = $this->model_user_user->getWhitelist($this->data['user']['username']);
            $this->data['user']['blacklist'] = $this->model_user_user->getBlacklist($this->data['user']['username']);

            $this->data['policies'] = $this->model_policy_policy->getPolicies();
         }
      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }




      $this->render();
   }


   private function validate() {

      if(!isset($this->request->post['policy_group']) || !is_numeric($this->request->post['policy_group']) || $this->request->post['policy_group'] < 0) {
         $this->error['policy_group'] = $this->data['text_invalid_policy_group'];
      }

      if(isset($this->request->post['password']) && strlen(@$this->request->post['password']) > 1) {

         if(strlen(@$this->request->post['password']) < 8 || strlen(@$this->request->post['password2']) < 8) {
            $this->error['password'] = $this->data['text_invalid_password'];
         }

         if($this->request->post['password'] != $this->request->post['password2']) {
            $this->error['password'] = $this->data['text_password_mismatch'];
         }
      }


      if(!isset($this->request->post['uid']) || !is_numeric($this->request->post['uid']) || (int)$this->request->post['uid'] < 0) {
         $this->error['uid'] = $this->data['text_invalid_uid'];
      }

      if(checkemail(@$this->request->post['email']) == 0 || checkemail(@$this->request->post['email_orig']) == 0) {
         $this->error['email'] = $this->data['text_invalid_email'];
      }

      if(!isset($this->request->post['username']) || strlen($this->request->post['username']) < 2 ) {
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
