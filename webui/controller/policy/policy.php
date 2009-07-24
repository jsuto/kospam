<?php


class ControllerPolicyPolicy extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "policy/list.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('policy/policy');

      $this->document->title = $this->data['text_policy'];


      $this->data['username'] = Registry::get('username');

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         /* get list of current policies */

         $this->data['policies'] = $this->model_policy_policy->getPolicies();
      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }


}

?>
