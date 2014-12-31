<?php

class ModelMailMail extends Model {


   public function send_smtp_email($host, $port, $yourdomain, $from, $to = array(), $msg){
      $ok = 0;

      if($to == "" || strlen($msg) < 30){ return $ok; }

      if(REWRITE_MESSAGE_ID == 1) {
         $msg = preg_replace("/Message-ID:([^\n]+)\n/i", "Message-ID: <" . generate_random_string(25) . '@' . SITE_NAME . ">\n", $msg);
      }

      $r = fsockopen($host, $port);
      if(!$r){ return -1; }

      $l = fgets($r, 4096);

      fputs($r, "HELO $yourdomain\r\n");
      $l = fgets($r, 4096);

      fputs($r, "MAIL FROM: <$from>\r\n");
      $l = fgets($r, 4096);

      while(list($k, $v) = each($to)) {
         fputs($r, "RCPT TO: <$v>\r\n");
         $l = fgets($r, 4096);
      }

      fputs($r, "DATA\r\n");
      $l = fgets($r, 4096);
      if(!preg_match("/^354/", $l)){ $l = fgets($r, 4096); }

      fputs($r, $msg);

      if(!preg_match("/\r\n\.\r\n$/", $msg)){
         fputs($r, "\r\n.\r\n");
      }

      $l = fgets($r, 4096);

      if(preg_match("/^250/", $l)){ $ok = 1; }

      fputs($r, "QUIT\r\n");
      $l = fgets($r, 4096);

      fclose($r);

      syslog(LOG_INFO, "sending mail from=$from, rcpt=" . implode(" ", $to) . ", status=$ok");

      return $ok;
   }


   public function message_as_rfc822_attachment($clapf_id = '', $sender = '', $rcpt = '', $msg = '') {
      if($clapf_id == '' || $sender == '' || $rcpt == '' || $msg == '') { return ''; }

      $boundary = generate_random_string(24);

      $s = "";
      $s .= "Date: " . date("r") . EOL;
      $s .= "Message-ID: <" . generate_random_string(25) . '@' . SITE_NAME . ">" . EOL;
      $s .= "From: " . $sender . EOL;
      $s .= "To: " . $rcpt . EOL;
      $s .= "Subject: forwarded email" . EOL;
      $s .= "MIME-Version: 1.0" . EOL;
      $s .= "Content-Type: multipart/mixed; boundary=\"$boundary\"" . EOL . EOL . EOL;
      $s .= "--$boundary" . EOL;
      $s .= "Content-Type: message/rfc822; name=\"" . $clapf_id . "\"" . EOL;
      $s .= "Content-Disposition: attachment; filename=\"" . $clapf_id . "\"" . EOL . EOL;
      $s .= $msg . EOL;
      $s .= "--$boundary" . EOL;

      return $s;
   }


   public function connect_imap() {
      $this->imap = new Zend_Mail_Protocol_Imap(IMAP_HOST, IMAP_PORT, IMAP_SSL);

      $session = Registry::get('session');

      if($this->imap) {
         if($this->imap->login($session->get("username"), $session->get("password"))) { return 1; }
      }   

      return 0;
   }


   public function disconnect_imap() {
      $this->imap->logout;
   }


}

?>
