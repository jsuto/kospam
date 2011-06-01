<?php


class ControllerImportImport extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "import/import.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');
      $language = Registry::get('language');

      $this->load->model('user/user');
      $this->load->model('user/import');

      $this->document->title = $language->get('text_import_users');


      $users = array();


      /* check if we are admin */

      if(Registry::get('admin_user') == 1 || Registry::get('domain_admin') == 1) {

         if($this->request->server['REQUEST_METHOD'] == 'POST' && $this->validate() == true) {
            $this->template = "import/imported.tpl";

            $users = $this->model_user_import->queryRemoteUsers($this->request->post);
            list($this->data['n'], $a) = $this->model_user_import->processUsers($users, $this->request->post);

            $rc = $this->model_user_import->fillRemoteTable($this->request->post, $this->request->post['domain']);

         }

      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }


   private function checkdomain($domain = '') {
      if($domain == '') { return 0; }

      $domains = $this->model_user_user->getEmailDomains();

      if(in_array($domain, $domains)) { return 1; }

      return 0;
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

      if(!isset($this->request->post['domain']) || strlen($this->request->post['domain']) < 2 || $this->checkdomain($this->request->post['domain']) == 0) {
         $this->error['domain'] = $this->data['text_invalid_data'];
      }

      if(!isset($this->request->post['policy_group']) || !is_numeric($this->request->post['policy_group']) || $this->request->post['policy_group'] < 0) {
         $this->error['policy_group'] = $this->data['text_invalid_policy_group'];
      }

      if(!isset($this->request->post['gid']) || !is_numeric($this->request->post['gid']) || $this->request->post['gid'] < 1) {
         $this->error['policy_group'] = $this->data['text_invalid_gid'];
      }

      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }


}

?>
