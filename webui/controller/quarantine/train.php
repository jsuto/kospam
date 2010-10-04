<?php


class ControllerQuarantineTrain extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/deliver.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('quarantine/database');
      $this->load->model('mail/mail');

      if(DB_DRIVER == "ldap")
         $this->load->model('user/ldap/user');
      else
         $this->load->model('user/sql/user');

      $this->document->title = $this->data['text_quarantine'];


      $this->data['id'] = @$this->request->get['id'];
      $this->data['from'] = @$this->request->get['from'];
      $this->data['subj'] = @$this->request->get['subj'];
      $this->data['hamspam'] = @$this->request->get['hamspam'];
      $this->data['page'] = @$this->request->get['page'];

      $this->data['globaltrain'] = 0;

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

         if(Registry::get('admin_user') == 1 && $_SESSION['train_global']) {
            touch($my_q_dir . "/" . preg_replace("/^[sh]\./", "g.", $this->data['id']) );
            $this->data['globaltrain'] = 1;
         }

         /* assemble training message */

         if($this->data['id'][0] == 's') {
            $training_address = HAM_TRAIN_ADDRESS;
         }
         else {
            $training_address = SPAM_TRAIN_ADDRESS;
         }

         $training_message  = "From: " . $fromaddr . "\r\nTo: " . $training_address . "\r\nSubject: training message\r\n\r\n\r\n";
         $training_message .= "Received: " . preg_replace("/^[sh]\./", "", $this->data['id']) . "\r\n" . $message;

         $x = $this->model_mail_mail->SendSmtpEmail(SMTP_HOST, POSTFIX_PORT, SMTP_DOMAIN, $fromaddr, $training_address, $training_message);

         if($x == 1){

            $Q = new DB("sqlite", "", "", "", $my_q_dir . "/" . QUARANTINE_DATA, "");
            Registry::set('Q', $Q);

            $this->model_quarantine_database->RemoveEntry($this->data['id']);

            /* deliver only the the false positives */

            if($this->data['id'][0] == 's' && (int)@$this->request->get['nodeliver'] == 0) {
               header("Location: " . SITE_URL . "/index.php?route=quarantine/deliver&id=" . $this->data['id'] . "&page=" . (int)$this->data['page'] . "&from=" . $this->data['from'] . "&subj=" . $this->data['subj'] . "&hamspam=" . $this->data['hamspam'] . "&user=" . $this->data['username'] . "&globaltrain=" . $this->data['globaltrain']);
            }
            else {
               $this->data['x'] = $this->data['text_successfully_trained'];
            }

         }
         else {
            $this->data['x'] = $this->data['text_message_failed_to_train'];
         }
      }


      $this->render();
   }


}

?>
