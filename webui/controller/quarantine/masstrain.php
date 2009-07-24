<?php


class ControllerQuarantineMasstrain extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/masstrain.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('user/user');
      $this->load->model('mail/mail');

      $this->document->title = $this->data['text_quarantine'];


      $this->data['username'] = Registry::get('username');

      $this->data['n'] = 0;

      /* fix username if we are admin */

      if(Registry::get('admin_user') == 1 && isset($this->request->get['user']) && strlen($this->request->get['user']) > 1) {
         $this->data['username'] = $this->request->get['user'];
      }

      $my_q_dir = QUEUE_DIRECTORY . "/" . substr($this->data['username'], 0, 1) . "/" . $this->data['username'];

      $fromaddr = $this->data['to'] = $this->model_user_user->getEmailAddress($this->data['username']);

      while(list($k, $v) = each($_POST)){
         if(preg_match("/^[a-f0-9]{28,36}$/", $k)){

            $message = $this->model_quarantine_message->getMessageForDelivery($my_q_dir . "/" . "s." . $k);

            /* assemble training message */

            $training_message  = "From: " . $fromaddr . "\r\nTo: " . HAM_TRAIN_ADDRESS . "\r\nSubject: training a spam as ham\r\n\r\n\r\n";
            $training_message .= CLAPF_HEADER_FIELD . $k . "\r\n" . $message;

            $x = $this->model_mail_mail->SendSmtpEmail(SMTP_HOST, CLAPF_PORT, SMTP_DOMAIN, $fromaddr, HAM_TRAIN_ADDRESS, $training_message);

            if($x == 1 && $this->model_mail_mail->SendSmtpEmail(SMTP_HOST, SMTP_PORT, SMTP_DOMAIN, SMTP_FROMADDR, $this->data['to'], $message) == 1) {

               if(file_exists($my_q_dir . "/" . "s." . $k)){
                  unlink($my_q_dir . "/" . "s." . $k);
               }

               $this->data['n']++;
            }
         }
      }



      $this->render();
   }


}

?>
