<?php


class ControllerPolicyView extends Controller {
   private $error = array();

   public function index(){
      $this->data['policy_group'] = 0;

      $this->id = "content";
      $this->template = "policy/view.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('policy/policy');

      $this->document->title = $this->data['text_policy'];


      $this->data['username'] = Registry::get('username');

      $this->data['policy_group'] = (int)@$this->request->get['policy_group'];

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         /* get policy settings */

         $this->data['policy'] = $this->model_policy_policy->getPolicy($this->data['policy_group']);

      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }


}

?>
