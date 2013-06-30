<?php


class ControllerQuarantineDeliver extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/deliver.tpl";
      $this->layout = "common/layout-empty";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('quarantine/database');

      $this->load->model('user/user');
      $this->load->model('mail/mail');

      $this->document->title = $this->data['text_quarantine'];

      $this->delivered = $this->trained = 0;

      if(isset($this->request->get['id'])) { $this->data['id'] = $this->request->get['id']; }

      $this->data['username'] = Registry::get('username');

      /* fix username if we are admin */

      if(isset($this->request->get['user']) && strlen($this->request->get['user']) > 1 && (Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->get['user']) == 1) ) {
         $this->data['username'] = $this->request->get['user'];
      }

      /* the same fix with POST requests */

      if(isset($this->request->post['user']) && strlen($this->request->post['user']) > 1 && (Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->post['user']) == 1) ) {
         $this->data['username'] = $this->request->post['user'];
      }

      $uid = $this->model_user_user->getUidByName($this->data['username']);
      $domain = $this->model_user_user->getDomainsByUid($uid);
      $my_q_dir = get_per_user_queue_dir($domain[0], $this->data['username'], $uid);

      $Q = Registry::get('Q');

      if( (Registry::get('admin_user') == 1 || Registry::get('domain_admin') == 1) && (!isset($this->request->get['user']) || strlen($this->request->get['user']) < 1) ) {
         $this->data['username'] = '';
      }


      $this->data['n'] = -1;


      /* deliver selected messages */

      if(isset($this->request->post['ids'])){

         $ids = preg_split("/ /", $this->request->post['ids']);

         while(list($k, $v) = each($ids)){

            $a = preg_split("/\+/", $v);
            if(count($a) == 2) {
               $id = $a[0];

               $username = preg_replace("/\*/", ".", $a[1]);

               $uid = $this->model_user_user->getUidByName($username);
               $domain = $this->model_user_user->getDomainsByUid($uid);
               $my_q_dir = get_per_user_queue_dir($domain[0], $username, $uid);

               $rcpt = $this->model_user_user->getEmailAddress($username);

               if($this->model_quarantine_message->checkId($id) && strlen($rcpt) > 3) {

                  $message = $this->model_quarantine_message->getMessageForDelivery($my_q_dir . "/" . $id);

                  $rc = 0;

                  if(TRAIN_DELIVERED_SPAM == 1) {
                     $rc = $this->send_training_message($my_q_dir, $id, $rcpt, $message);
                  }

                  if(TRAIN_DELIVERED_SPAM == 0 || (TRAIN_DELIVERED_SPAM == 1 && $rc == 1 && ($id[0] == 's' || $id[0] == 'v')) ) {
                     $this->deliver_message($my_q_dir, $uid, $id, $rcpt, $message);
                  }

               }

            }

         }

         if(TRAIN_DELIVERED_SPAM == 1) {
            $this->data['message'] = $this->data['text_successfully_trained'];
            $this->data['n'] = $this->trained;
         }
         else {
            $this->data['message'] = $this->data['text_successfully_delivered'];
            $this->data['n'] = $this->delivered;
         }
      }


      /* deliver one message */

      if(isset($this->request->get['id'])) {

         $this->data['message'] = $this->data['text_failed_to_deliver'];
         $rcpt = $this->model_user_user->getEmailAddress($this->data['username']);

         if($this->model_quarantine_message->checkId($this->data['id']) && strlen($rcpt) > 3){

            $message = $this->model_quarantine_message->getMessageForDelivery($my_q_dir . "/" . $this->data['id']);

            $rc = 0;

            if(TRAIN_DELIVERED_SPAM == 1) {
               $rc = $this->send_training_message($my_q_dir, $this->data['id'], $rcpt, $message);
            }

            if(TRAIN_DELIVERED_SPAM == 0 || (TRAIN_DELIVERED_SPAM == 1 && $rc == 1 && ($this->data['id'][0] == 's' || $this->data['id'][0] == 'v')) ) {
               $this->deliver_message($my_q_dir, $uid, $this->data['id'], $rcpt, $message);
            }

         }
      }



      $this->render();
   }




   private function send_training_message($my_q_dir = '', $id = '', $fromaddr = '', $message = '') {
      $rc = 0;

      if($id == '' || strlen($fromaddr) < 3 || strlen($message) < 10) { return 0; }

      if(Registry::get('admin_user') == 1 && $_SESSION['train_global']) {
         touch($my_q_dir . "/" . preg_replace("/^[sh]\./", "g.", $id) );
         $this->data['globaltrain'] = 1;
      }

      if($id[0] == 's') {
         $training_address = HAM_TRAIN_ADDRESS;
      } else {
         $training_address = SPAM_TRAIN_ADDRESS;
      }

      if($id == 'v') {
         $rc = 1;
      }
      else {
         $rc = $this->model_mail_mail->SendSmtpEmail(
                                                    POSTFIX_LISTEN_ADDRESS,
                                                    POSTFIX_LISTEN_PORT,
                                                    SMTP_DOMAIN,
                                                    $fromaddr,
                                                    $training_address,
                                                    "From: " . $fromaddr . "\r\nTo: " . $training_address . "\r\nSubject: training message\r\n\r\n\r\n" . "Received: " . preg_replace("/^[sh]\./", "", $id) . "\r\n" . $message
                                                  );
      }


      if($rc == 1) {
         $this->data['message'] = $this->data['text_successfully_trained'];
         $this->trained++;
      }
      else {
         $this->data['message'] = $this->data['text_message_failed_to_train'];
      }

      return $rc;
   }


   private function deliver_message($my_q_dir = '', $uid = 0, $id = '', $rcpt = '', $message = '') {

      $rc = $this->model_mail_mail->SendSmtpEmail(LOCALHOST, POSTFIX_PORT_AFTER_CONTENT_FILTER, SMTP_DOMAIN, SMTP_FROMADDR, $rcpt, $message);

      if($rc == 1){
         $this->model_quarantine_database->RemoveEntry($id, $uid);

         if(REMOVE_FROM_QUARANTINE_WILL_UNLINK_FROM_FILESYSTEM == 1) {
            unlink($my_q_dir . "/" . $id);
         }

         $this->delivered++;

         $this->data['message'] = $this->data['text_successfully_delivered'];
      }

      return $rc;
   }


}

?>
