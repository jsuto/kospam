<?php


class ControllerUserEmail extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "user/email.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('user/user');

      $this->document->title = $this->data['text_user_management'];


      $this->data['uid'] = (int)@$this->request->get['uid'];
      $this->data['email'] = @$this->request->get['email'];
      $this->data['username'] = @$this->request->get['username'];

      if($this->request->server['REQUEST_METHOD'] == 'POST') {
         if($this->validate() == true) {
            $ret = $this->model_user_user->addEmail($this->request->post['uid'], $this->request->post['email']);
            if($ret == 1){
               $this->data['x'] = $this->data['text_successfully_added'];
            }
            else {
               $this->data['x'] = $this->data['text_failed_to_add'];
            }
         }
         else {
            $this->template = "common/error.tpl";
            $this->data['errorstring'] = array_pop($this->error);
         }
      }


      $this->render();
   }


   private function validate() {

      if(Registry::get('admin_user') == 0) {
         $this->error['admin'] = $this->data['text_you_are_not_admin'];
      }

      if(!isset($this->request->post['uid']) || !is_numeric($this->request->post['uid']) || $this->request->post['uid'] < 1 ) {
         $this->error['uid'] = $this->data['text_invalid_uid'];
      }

      if(checkemail($this->request->post['email']) == 0) {
         $this->error['email'] = $this->data['text_invalid_email'];
      }

      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }


}

?>
