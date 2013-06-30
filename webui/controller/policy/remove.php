<?php


class ControllerPolicyRemove extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "policy/remove.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('policy/policy');

      $this->document->title = $this->data['text_policy'];


      $this->data['username'] = Registry::get('username');

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         if(isset($this->request->get['policy_group']) && isset($this->request->get['confirmed']) && $this->request->get['confirmed'] == 1) {

            if($this->model_policy_policy->removePolicy($this->request->get['policy_group']) == 1) {
               $this->data['x'] = $this->data['text_successfully_removed'];
               header("Location: index.php?route=policy/policy");
               exit;
            }
            else {
               $this->data['x'] = $this->data['text_failed_to_remove'];
            }
         }
         else {
            $this->data['x'] = $this->data['text_error'];
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
