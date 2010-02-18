<?php


class ControllerUserRemove extends Controller {
   private $error = array();
   private $domains = array();
   private $d = array();

   public function index(){

      $this->id = "content";
      $this->template = "user/remove.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      if(DB_DRIVER == "ldap")
         $this->load->model('user/ldap/user');
      else
         $this->load->model('user/sql/user');

      $this->document->title = $this->data['text_policy'];


      $this->data['username'] = Registry::get('username');

      $this->data['uid'] = (int)@$this->request->get['uid'];
      $this->data['user'] = @$this->request->get['user'];
      $this->data['confirmed'] = (int)@$this->request->get['confirmed'];

      $this->data['domains'] = $this->model_user_user->getDomains();

      $this->domains = $this->model_user_user->getEmailDomains();

      if($this->validate() == true) {

         if($this->data['confirmed'] == 1) {
            $ret = $this->model_user_user->deleteUser($this->data['uid']);
            if($ret == 1){
               $this->data['x'] = $this->data['text_successfully_removed'];
            }
            else {
               $this->data['x'] = $this->data['text_failed_to_remove'];
            }
         }
      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = array_pop($this->error);
      }



      $this->render();
   }


   private function validate() {

      if(Registry::get('admin_user') == 0 && Registry::get('domain_admin') == 0) {
         $this->error['admin'] = $this->data['text_you_are_not_admin'];
      }

      if(!isset($this->request->get['uid']) || !is_numeric($this->request->get['uid']) || $this->request->get['uid'] < 1 ) {
         $this->error['username'] = $this->data['text_invalid_uid'];
      }



      /* apply additional restrictions on domain admins */

      if(Registry::get('domain_admin') == 1) {
         if($this->model_user_user->isUidInMyDomain((int)@$this->request->get['uid']) == 0) {
            $this->error['uid'] = $this->data['text_invalid_uid'];
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
