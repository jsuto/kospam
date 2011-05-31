<?php


class ControllerQuarantineDeliver extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/deliver.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('quarantine/database');
      $this->load->model('mail/mail');

      $this->load->model('user/user');

      $this->document->title = $this->data['text_quarantine'];


      $this->data['id'] = @$this->request->get['id'];
      $this->data['from'] = @$this->request->get['from'];
      $this->data['subj'] = @$this->request->get['subj'];
      $this->data['hamspam'] = @$this->request->get['hamspam'];
      $this->data['page'] = @$this->request->get['page'];
      $this->data['globaltrain'] = @$this->request->get['globaltrain'];

      $this->data['username'] = Registry::get('username');

      /* fix username if we are admin */

      if(isset($this->request->get['user']) && strlen($this->request->get['user']) > 1 && (Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->get['user']) == 1) ) {
         $this->data['username'] = $this->request->get['user'];
      }


      $uid = $this->model_user_user->getUidByName($this->data['username']);
      $domain = $this->model_user_user->getDomainsByUid($uid);
      $my_q_dir = get_per_user_queue_dir($domain[0], $this->data['username'], $uid);


      $this->data['x'] = $this->data['text_failed_to_deliver'];
      $this->data['to'] = $this->model_user_user->getEmailAddress($this->data['username']);

      $Q = Registry::get('Q');

      if($this->model_quarantine_message->checkId($this->data['id']) && strlen($this->data['to']) > 3){
         $message = $this->model_quarantine_message->getMessageForDelivery($my_q_dir . "/" . $this->data['id']);

         $x = $this->model_mail_mail->SendSmtpEmail(LOCALHOST, POSTFIX_PORT_AFTER_CONTENT_FILTER, SMTP_DOMAIN, SMTP_FROMADDR, $this->data['to'], $message);

         if($x == 1){

            $this->model_quarantine_database->RemoveEntry($this->data['id']);

            if(REMOVE_FROM_QUARANTINE_WILL_UNLINK_FROM_FILESYSTEM == 1) { unlink($my_q_dir . "/" . $this->data['id']); }

            $this->data['x'] = $this->data['text_successfully_delivered'];
         }
      }


      $this->render();
   }


}

?>
