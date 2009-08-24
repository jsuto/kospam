<?php


class ControllerQuarantineMessage extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/message.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('user/user');

      $this->document->title = $this->data['text_quarantine'];

      $this->data['id'] = @$this->request->get['id'];
      $this->data['raw'] = (int)@$this->request->get['raw'];

      

      $this->data['username'] = Registry::get('username');

      /* fix username if we are admin */

      if(Registry::get('admin_user') == 1 && isset($this->request->get['user']) && strlen($this->request->get['user']) > 1) {
         $this->data['username'] = $this->request->get['user'];
      }

      $my_q_dir = get_per_user_queue_dir($this->model_user_user->getUidByName($this->data['username']));


      if($this->data['raw'] == 1){
         $this->data['message'] = $this->model_quarantine_message->ShowRawMessage($my_q_dir, $this->data['id']);
      } else {
         $this->data['message'] = $this->model_quarantine_message->ShowMessage($my_q_dir, $this->data['id']);
      }


      $this->render();
   }


}

?>
