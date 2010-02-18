<?php


class ControllerPolicyRemove extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "policy/remove.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      if(DB_DRIVER == "ldap")
         $this->load->model('policy/ldap/policy');
      else
         $this->load->model('policy/sql/policy');

      $this->document->title = $this->data['text_policy'];


      $this->data['username'] = Registry::get('username');

      $this->data['policy_group'] = (int)@$this->request->get['policy_group'];
      $this->data['confirmed'] = (int)@$this->request->get['confirmed'];

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         if($this->data['confirmed'] == 1) {
            if($this->model_policy_policy->removePolicy($this->data['policy_group']) == 1) {
               $this->data['x'] = $this->data['text_successfully_removed'];
            }
            else {
               $this->data['x'] = $this->data['text_failed_to_remove'];
            }
         }
         else {
            $query = $this->model_policy_policy->getPolicy($this->data['policy_group']);
            $this->data['policy_name'] = $query['name'];
         }
      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }


}

?>
