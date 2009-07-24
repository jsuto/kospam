<?php


class ControllerQuarantineDeliver extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/deliver.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('user/user');
      $this->load->model('mail/mail');

      $this->document->title = $this->data['text_quarantine'];


      $this->data['id'] = @$this->request->get['id'];


      $this->data['username'] = Registry::get('username');

      /* fix username if we are admin */

      if(Registry::get('admin_user') == 1 && isset($this->request->get['user']) && strlen($this->request->get['user']) > 1) {
         $this->data['username'] = $this->request->get['user'];
      }

      $my_q_dir = QUEUE_DIRECTORY . "/" . substr($this->data['username'], 0, 1) . "/" . $this->data['username'];


      $this->data['x'] = $this->data['text_failed_to_deliver'];
      $this->data['to'] = $this->model_user_user->getEmailAddress($this->data['username']);

      if($this->model_quarantine_message->checkId($this->data['id']) && strlen($this->data['to']) > 3){
         $message = $this->model_quarantine_message->getMessageForDelivery($my_q_dir . "/" . $this->data['id']);

         $x = $this->model_mail_mail->SendSmtpEmail(SMTP_HOST, SMTP_PORT, SMTP_DOMAIN, SMTP_FROMADDR, $this->data['to'], $message);

         if($x == 1){
            if(file_exists($my_q_dir . "/" . $this->data['id'])){
               unlink($my_q_dir . "/" . $this->data['id']);
            }

            $this->data['x'] = $this->data['text_successfully_delivered'];
         }
      }


      $this->render();
   }


}

?>
