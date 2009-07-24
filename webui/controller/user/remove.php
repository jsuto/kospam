<?php


class ControllerUserRemove extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "user/remove.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('user/user');

      $this->document->title = $this->data['text_policy'];


      $this->data['username'] = Registry::get('username');

      $this->data['uid'] = (int)@$this->request->get['uid'];
      $this->data['email'] = @$this->request->get['email'];
      $this->data['confirmed'] = (int)@$this->request->get['confirmed'];

      if($this->validate() == true) {

         if($this->data['confirmed'] == 1) {
            $ret = $this->model_user_user->deleteUser($this->data['uid'], $this->data['email']);
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

      if(Registry::get('admin_user') == 0) {
         $this->error['admin'] = $this->data['text_you_are_not_admin'];
      }

      if(checkemail(@$this->request->get['email']) == 0) {
         $this->error['email'] = $this->data['text_invalid_email'];
      }

      if(!isset($this->request->get['uid']) || !is_numeric($this->request->get['uid']) || $this->request->get['uid'] < 1 ) {
         $this->error['username'] = $this->data['text_invalid_uid'];
      }


      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }


}

?>
