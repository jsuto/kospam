<?php

class ModelMailMail extends Model {


   public function SendSmtpEmail($smtphost, $smtpport, $yourdomain, $from, $to, $msg){
      $ok = 0;

      if($to == "" || strlen($msg) < 30){ return $ok; }

      $r = fsockopen($smtphost, $smtpport);
      if(!$r){ return -1; }

      $l = fgets($r, 4096);

      fputs($r, "HELO $yourdomain\r\n");
      $l = fgets($r, 4096);

      fputs($r, "MAIL FROM: <$from>\r\n");
      $l = fgets($r, 4096);

      fputs($r, "RCPT TO: <$to>\r\n");
      $l = fgets($r, 4096);


      fputs($r, "DATA\r\n");
      $l = fgets($r, 4096);
      if(!preg_match("/^354/", $l)){ $l = fgets($r, 4096); }

      fputs($r, $msg);

      if(!preg_match("/\r\n.\r\n$/", $msg)){
         fputs($r, "/\r\n.\r\n");
      }

      $l = fgets($r, 4096);

      if(preg_match("/^250/", $l)){ $ok = 1; }

      fputs($r, "QUIT\r\n");
      $l = fgets($r, 4096);

      fclose($r);

      return $ok;
   }

}

?>
