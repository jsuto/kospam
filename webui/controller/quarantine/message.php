<?php


class ControllerQuarantineMessage extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/message.tpl";
      $this->layout = "common/layout-empty";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');

      $this->load->model('user/user');

      $this->document->title = $this->data['text_quarantine'];

      $this->data['id'] = '';
      $this->data['raw'] = 0;


      if(isset($this->request->get['id'])) { $this->data['id'] = $this->request->get['id']; }
      if(isset($this->request->get['raw'])) { $this->data['raw'] = $this->request->get['raw']; }


      $this->data['username'] = Registry::get('username');


/*
TODO: need username if you are admin!!!!!!!!
*/


      /* fix username if we are admin */

      if(isset($this->request->get['user']) && strlen($this->request->get['user']) > 1 && (Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->get['user']) == 1) ) {
         $this->data['username'] = $this->request->get['user'];
      }

      $uid = $this->model_user_user->getUidByName($this->data['username']);
      $domain = $this->model_user_user->getDomainsByUid($uid);
      $my_q_dir = get_per_user_queue_dir($domain[0], $this->data['username'], $uid);

      $Q = Registry::get('Q');

      if($this->data['raw'] == 1){
         $this->data['message'] = $this->model_quarantine_message->ShowRawMessage($my_q_dir, $this->data['id']);
      } else {
         $this->data['message'] = $this->model_quarantine_message->show_message($my_q_dir, $this->data['id']);
      }


      $this->render();
   }


}

?>
