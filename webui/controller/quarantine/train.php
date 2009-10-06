<?php


class ControllerQuarantineTrain extends Controller {

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

      if(isset($this->request->get['user']) && strlen($this->request->get['user']) > 1 && (Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->get['user']) == 1) ) {
         $this->data['username'] = $this->request->get['user'];
      }

      $uid = $this->model_user_user->getUidByName($this->data['username']);
      $domain = $this->model_user_user->getDomainsByUid($uid);
      $my_q_dir = get_per_user_queue_dir($domain[0], $this->data['username'], $uid);


      $this->data['x'] = $this->data['text_failed_to_deliver'];
      $fromaddr = $this->model_user_user->getEmailAddress($this->data['username']);

      if($this->model_quarantine_message->checkId($this->data['id']) && strlen($fromaddr) > 3){
         $message = $this->model_quarantine_message->getMessageForDelivery($my_q_dir . "/" . $this->data['id']);

         /* assemble training message */

         $training_message  = "From: " . $fromaddr . "\r\nTo: " . HAM_TRAIN_ADDRESS . "\r\nSubject: training a spam as ham\r\n\r\n\r\n";
         $training_message .= "Received: " . preg_replace("/^s\./", "", $this->data['id']) . "\r\n" . $message;

         $x = $this->model_mail_mail->SendSmtpEmail(SMTP_HOST, CLAPF_PORT, SMTP_DOMAIN, $fromaddr, HAM_TRAIN_ADDRESS, $training_message);

         if($x == 1){

            /* now deliver it by calling the delivery url */

            header("Location: " . SITE_URL . "/index.php?route=quarantine/deliver&id=" . $this->data['id'] . "&user=" . $this->data['username']);
         }
         else {
            $this->data['x'] = $this->data['text_message_failed_to_train'];
         }
      }


      $this->render();
   }


}

?>
