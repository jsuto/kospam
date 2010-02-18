<?php


class ControllerUserAdd extends Controller {
   private $error = array();
   private $domains = array();

   public function index(){

      $this->id = "content";
      $this->template = "user/add.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      if(DB_DRIVER == "ldap"){
         $this->load->model('user/ldap/user');
         $this->load->model('policy/ldap/policy');
      } else {
         $this->load->model('user/sql/user');
         $this->load->model('policy/sql/policy');
      }


      $this->document->title = $this->data['text_user_management'];

      $this->data['domains'] = array();

      /* check if we are admin */

      if(Registry::get('admin_user') == 1 || Registry::get('domain_admin') == 1) {

         /* query available domains */

         $this->data['domains'] = $this->model_user_user->getDomains();

         $this->domains = $this->model_user_user->getEmailDomains();


         if($this->request->server['REQUEST_METHOD'] == 'POST') {
            $ret = 0;

            if($this->validate() == true){
               $ret = $this->model_user_user->addUser($this->request->post);

               if($ret == 1){
                  $this->data['x'] = $this->data['text_successfully_added'];
               } else {
                  $this->data['errorstring'] = $this->data['text_failed_to_add'];
               }
            }
            else {
               $this->data['errorstring'] = array_pop($this->error);

            }

            if($ret == 0) {

               $this->data['policies'] = $this->model_policy_policy->getPolicies();
               $this->data['post'] = $this->request->post;
               $this->data['next_user_id'] = $this->model_user_user->getNextUid();

            }
         }
         else {
            $this->data['policies'] = $this->model_policy_policy->getPolicies();
            $this->data['next_user_id'] = $this->model_user_user->getNextUid();
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

      if(!isset($this->request->post['password']) || !isset($this->request->post['password2']) ) {
         $this->error['password'] = $this->data['text_missing_password'];
      }

      if(strlen(@$this->request->post['password']) < MIN_PASSWORD_LENGTH || strlen(@$this->request->post['password2']) < MIN_PASSWORD_LENGTH) {
         $this->error['password'] = $this->data['text_too_short_password'];
      }

      if($this->request->post['password'] != $this->request->post['password2']) {
         $this->error['password'] = $this->data['text_password_mismatch'];
      }

      if(!isset($this->request->post['uid']) || !is_numeric($this->request->post['uid']) || $this->request->post['uid'] < 0) {
         $this->error['uid'] = $this->data['text_invalid_uid'];
      }

      if(!isset($this->request->post['email']) || strlen($this->request->post['email']) < 3) {
         $this->error['email'] = $this->data['text_invalid_email'];
      }
      else {
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

      if(!isset($this->request->post['username']) || strlen($this->request->post['username']) < 2) {
         $this->error['username'] = $this->data['text_invalid_username'];
      }

      if(isset($this->request->post['username']) && $this->model_user_user->getUidByName($this->request->post['username']) > 0) {
         $this->error['username'] = $this->data['text_existing_user'];
      }

      if(!isset($this->request->post['domain'])) {
         $this->error['domain'] = $this->data['text_missing_data'];
      }


      /* apply additional restrictions on domain admins */

      if(Registry::get('domain_admin') == 1) {
         if(@$this->request->post['domain'] != $this->data['domains'][0]) {
            $this->error['domain'] = $this->data['text_unauthorized_domain'];
         }

         if((int)@$this->request->post['isadmin'] == 1) {
            $this->error['isadmin'] = $this->data['text_missing_data'];
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
