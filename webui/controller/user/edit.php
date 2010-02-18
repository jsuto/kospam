<?php


class ControllerUserEdit extends Controller {
   private $error = array();
   private $domains = array();

   public function index(){
      $this->data['uid'] = 0;
      $this->data['email'] = "";

      $this->id = "content";
      $this->template = "user/edit.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');
      $language = Registry::get('language');

      if(DB_DRIVER == "ldap"){
         $this->load->model('user/ldap/user');
         $this->load->model('policy/ldap/policy');
      } else {
         $this->load->model('user/sql/user');
         $this->load->model('policy/sql/policy');
      }

      $this->document->title = $language->get('text_user_management');

      $this->data['domains'] = array();


      if(isset($this->request->get['uid']) && is_numeric($this->request->get['uid']) && $this->request->get['uid'] > 0) {
         $this->data['uid'] = $this->request->get['uid'];
      }

      if(isset($this->request->post['uid']) && is_numeric($this->request->post['uid']) && $this->request->post['uid'] > 0) {
         $this->data['uid'] = $this->request->post['uid'];
      }


      $this->domains = $this->model_user_user->getEmailDomains();


      if(isset($this->request->get['email']) && checkemail($this->request->get['email'], $this->domains) == 1) {
         $this->data['email'] = $this->request->get['email'];
      }


      /* check if we are admin */

      if(Registry::get('admin_user') == 1 || (Registry::get('domain_admin') == 1 && $this->model_user_user->isUidInMyDomain((int)@$this->data['uid']) == 1) ) {

         $this->data['domains'] = $this->model_user_user->getDomains();

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
            $this->data['emails'] = $this->model_user_user->getEmails($this->data['user']['username']);

            if(DB_DRIVER == 'ldap') {
               $this->data['email'] = $this->data['user']['email'];
            }

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

         if(strlen(@$this->request->post['password']) < MIN_PASSWORD_LENGTH || strlen(@$this->request->post['password2']) < MIN_PASSWORD_LENGTH) {
            $this->error['password'] = $this->data['text_invalid_password'];
         }

         if($this->request->post['password'] != $this->request->post['password2']) {
            $this->error['password'] = $this->data['text_password_mismatch'];
         }
      }

      if(!isset($this->request->post['uid']) || !is_numeric($this->request->post['uid']) || (int)$this->request->post['uid'] < 0) {
         $this->error['uid'] = $this->data['text_invalid_uid'];
      }

      if(strlen(@$this->request->post['email']) < 4) {
         $this->error['email'] = $this->data['text_invalid_email'];
      } else {
         $emails = explode("\n", $this->request->post['email']);
         foreach ($emails as $email) {
            $email = rtrim($email);
            $ret = checkemail($email, $this->domains);
            if($ret == 0) {
               $this->error['email'] = $this->data['text_invalid_email'] . ": $email";
            }
            else if($ret == -1) {
               $this->error['email'] = $this->data['text_email_in_unknown_domain'] . ": $email";
            }
         }
      }

      if(!isset($this->request->post['username']) || strlen($this->request->post['username']) < 2 ) {
         $this->error['username'] = $this->data['text_invalid_username'];
      }


      /* apply additional restrictions on domain admins */

      if(Registry::get('domain_admin') == 1) {
         if(@$this->request->post['domain'] != $this->data['domains'][0]) {
            $this->error['domain'] = $this->data['text_unauthorized_domain'];
         }

         if((int)@$this->request->post['isadmin'] == 1) {
            $this->error['isadmin'] = $this->data['text_invalid_data'];
         }

      }


      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }



}

?>
