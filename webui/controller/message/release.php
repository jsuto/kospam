<?php


class ControllerMessageRelease extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "message/headers.tpl";
      $this->layout = "common/layout-empty";

      $request = Registry::get('request');
      $db = Registry::get('db');
      $session = Registry::get('session');

      $this->load->model('search/search');
      $this->load->model('search/message');
      $this->load->model('user/user');
      $this->load->model('mail/mail');

      $this->document->title = $this->data['text_message'];

      $this->data['id'] = $this->request->post['id'];
      $this->data['spam'] = $this->request->post['spam'];

      $this->data['clapf_id'] = '';

      if(!verify_piler_id($this->data['id'])) {
         AUDIT(ACTION_UNKNOWN, '', '', $this->data['id'], 'unknown id: ' . $this->data['id']);
         die("invalid id: " . $this->data['id']);
      }

      if(!$this->model_search_search->check_your_permission_by_id($this->data['id'])) {
         AUDIT(ACTION_UNAUTHORIZED_VIEW_MESSAGE, '', '', $this->data['id'], '');
         die("no permission for " . $this->data['id']);
      }

      if($this->data['spam'] == 1) { AUDIT(ACTION_NOT_SPAM, '', '', $this->data['id'], ''); }
      else { AUDIT(ACTION_SPAM, '', '', $this->data['id'], ''); }


      /* release/deliver one message */


      if(isset($this->data['id'])) {

         $this->data['clapf_id'] = $this->model_search_message->get_clapf_id_by_id($this->data['id']);
         $message = 'Received: ' . $this->data['clapf_id'] . EOL . $this->model_search_message->get_raw_message($this->data['clapf_id']);

         /*
          * send a training message
          */

         if($this->data['spam'] == 1) {
            $train_address = HAM_TRAIN_ADDRESS;
         } else {
            $train_address = SPAM_TRAIN_ADDRESS;
         }

         $train_message = $this->model_mail_mail->message_as_rfc822_attachment($this->data['clapf_id'], $session->get("email"), $train_address, $message);

         $rc = $this->model_mail_mail->send_smtp_email(CLAPF_HOST, CLAPF_PORT, SMTP_DOMAIN, $session->get("email"), array($train_address), $train_message);

         if($rc == 1) {
            $this->model_search_search->hide_message($this->data['id']);
         }


         /*
          * in case of a false positive release the message to the recipient
          */

         if($this->data['spam'] == 1) {
            $rcpt = array();

            array_push($rcpt, $session->get("email"));

            $this->deliver_message($message, $rcpt);
         }

      }


   }


   private function deliver_message($message = '', $rcpt = array()) {

      $rc = $this->model_mail_mail->send_smtp_email(SMARTHOST, SMARTHOST_PORT, SMTP_DOMAIN, SMTP_FROMADDR, $rcpt, $message);

      if($rc == 1){
         $filename = $this->model_search_message->get_filename_by_clapf_id($this->data['clapf_id']);

         if(REMOVE_FROM_QUARANTINE_WILL_UNLINK_FROM_FILESYSTEM == 1 && $filename) {
            unlink($filename);
         }
      }

      return $rc;
   }


}

?>
