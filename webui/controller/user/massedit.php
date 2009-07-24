<?php


class ControllerUserMassedit extends Controller {
   private $error = array();

   public function index(){
      $this->data['uid'] = -1;
      $this->data['email'] = "";

      $this->id = "content";
      $this->template = "user/mass.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');
      $language = Registry::get('language');

      $this->load->model('user/bulk');
      $this->load->model('policy/policy');

      $this->document->title = $language->get('text_user_management');


      if(isset($this->request->get['uid']) && is_numeric($this->request->get['uid']) && $this->request->get['uid'] > 0) {
         $this->data['uid'] = $this->request->get['uid'];
      }

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         if($this->request->server['REQUEST_METHOD'] == 'POST') {

            /* bulk update */

            if((int)@$this->request->post['edit'] == 1 && $this->validate() == true) {
               $ret = $this->model_user_bulk->bulkUpdateUser($this->request->post['policy_group'], $this->request->post['whitelist'], $this->request->post['blacklist']);

               if($ret == 1){
                  $this->data['x'] = $this->data['text_successfully_modified'];
               }
               else {
                  $this->template = "common/error.tpl";
                  $this->data['x'] = $this->data['text_failed_to_modify'];
               }

            }


            /* bulk remove */

            else if((int)@$this->request->post['remove'] == 1) {
               $ret = $this->model_user_bulk->bulkDeleteUser();

               if($ret == 1){
                  $this->data['x'] = $this->data['text_successfully_removed'];
               }
               else {
                  $this->template = "common/error.tpl";
                  $this->data['x'] = $this->data['text_failed_to_remove'];
               }

            }

            else {
               $this->data['uidlist'] = $this->model_user_bulk->createUidList();
               $this->data['uids'] = explode(",", $this->data['uidlist']);

               $this->data['policies'] = $this->model_policy_policy->getPolicies();
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


      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }



}

?>
