<?php


class ControllerPolicyList extends Controller {
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

      /* get search term if there's any */

      $this->data['search'] = '';

      if($this->request->server['REQUEST_METHOD'] == 'POST' && isset($this->request->post['search'])){
         $this->data['search'] = $this->request->post['search'];
      }
      else if(isset($this->request->get['search'])) {
         $this->data['search'] = $this->request->get['search'];
      }


      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         /* get list of current policies */

         $this->data['policies'] = $this->model_policy_policy->get_policies();
      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }


}

?>
