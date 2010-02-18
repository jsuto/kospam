<?php


class ControllerImportQuery extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "import/query.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');
      $language = Registry::get('language');

      if(DB_DRIVER == "ldap") {
         $this->load->model('user/ldap/user');
         $this->load->model('user/ldap/import');
         $this->load->model('policy/ldap/policy');
      }
      else {
         $this->load->model('user/sql/user');
         $this->load->model('user/sql/import');
         $this->load->model('policy/sql/policy');
      }

      $this->document->title = $language->get('text_import_users');


      $users = array();


      /* check if we are admin */

      if(Registry::get('admin_user') == 1 || Registry::get('domain_admin') == 1) {

         if($this->request->server['REQUEST_METHOD'] == 'POST' && $this->validate() == true) {
            $this->template = "import/import.tpl";

            $this->data['users'] = $this->model_user_import->queryRemoteUsers($this->request->post);
            $this->data['policies'] = $this->model_policy_policy->getPolicies();

            $this->data['domains'] = $this->model_user_user->getDomains();

            $this->data['request'] = $this->request->post;
         }

         if($this->request->server['REQUEST_METHOD'] == 'GET') {
            $this->data['ldap'] = $this->model_user_import->getLdapParameters();
         }

      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }


   private function validate() {

      if(!isset($this->request->post['ldap_host']) || strlen($this->request->post['ldap_host']) < 2) {
         $this->error['ldap_host'] = $this->data['text_invalid_data'];
      }

      if(!isset($this->request->post['ldap_binddn']) || strlen($this->request->post['ldap_binddn']) < 2) {
         $this->error['ldap_binddn'] = $this->data['text_invalid_data'];
      }

      if(!isset($this->request->post['ldap_bindpw'])) {
         $this->error['ldap_bindpw'] = $this->data['text_invalid_data'];
      }

      if(!isset($this->request->post['ldap_basedn']) || strlen($this->request->post['ldap_basedn']) < 2) {
         $this->error['ldap_basedn'] = $this->data['text_invalid_data'];
      }


      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }


}

?>
