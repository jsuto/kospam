<?php

class ModelQuarantineMessage extends Model {


   public function checkId($id = '') {
      if(strlen($id) > 10 && preg_match('/^([shv]\.[0-9a-f]+)$/', $id)){
         return 1;
      }

      return 0;
   }


   public function getMessageForDelivery($f){
      $message = "";

      if(!file_exists($f)){ return ""; }

      $fp = fopen($f, "r");
      if($fp){
         while(($l = fgets($fp, 4096))){
            $message .= $l;
         }
         fclose($fp);
      }

      return $message;
   }


   public function ShowRawMessage($dir = '', $id = '') {
      $message = "";

      if($dir == "" || $id == "" || !$this->checkId($id) || !file_exists($dir . "/" . $id)){ return ""; }

      $fp = fopen($dir . "/" . $id, "r");
      if($fp){
         while(($l = fgets($fp, 4096))){
            $message .= $l;
         }

         fclose($fp);
      }

      $message = htmlentities($message);

      return "<pre>\n" . $this->print_nicely($message) . "</pre>\n";
   }


   public function show_message($dir = '', $id = '') {
      $header = "";
      $body_chunk = "";
      $is_header = 1;
      $state = "UNDEF";
      $b = array();
      $boundary = array();
      $text_plain = 1;
      $text_html = 0;
      $charset = "";
      $qp = $base64 = 0;
      $has_text_plain = 0;
      $rfc822 = 0;
      $_1st_header = 1;

      $from = $to = $subject = $date = $message = "";

      if($dir == "" || $id == "" || !$this->checkId($id) || !file_exists($dir . "/" . $id) || !is_readable($dir . "/" . $id) ){ return ""; }

      $fp = fopen($dir . "/" . $id, "r");
      if($fp){
         while(($l = fgets($fp, 4096))){

            if(($l[0] == "\r" && $l[1] == "\n" && $is_header == 1) || ($l[0] == "\n" && $is_header == 1) ){
               $is_header = $_1st_header = 0;

               if($rfc822 == 1) { $rfc822 = 0; $is_header = 1; }

            }

            if($is_header == 1 && preg_match("/^Content-Type:/i", $l)) $state = "CONTENT_TYPE";
            if($is_header == 1 && preg_match("/^Content-Transfer-Encoding:/i", $l)) $state = "CONTENT_TRANSFER_ENCODING";

            if($state == "CONTENT_TYPE") {

               $x = strstr($l, "boundary");
               if($x){
                  $a = explode(";", $x);
                  $x = $a[0];

                  $x = preg_replace("/boundary =/", "boundary=", $x);
                  $x = preg_replace("/boundary= /", "boundary=", $x);

                  $x = preg_replace("/\"\;{0,1}/", "", $x);
                  $x = preg_replace("/\'/", "", $x);

                  $b = explode("boundary=", $x);
                  array_push($boundary, rtrim($b[count($b)-1]));
               }

               if(preg_match("/charset/i", $l)){
                  $types = explode(";", $l);
                  foreach ($types as $type){
                     if(preg_match("/charset/i", $type)){
                        $type = preg_replace("/[\"\'\ ]/", "", $type);

                        $x = explode("=", $type);
                        $charset = $x[1];
                     }
                  }
               }

               if(strstr($l, "message/rfc822")) { $rfc822 = 1; }

               if(strstr($l, "text/plain")){ $text_plain = 1; $has_text_plain = 1; }
               if(strstr($l, "text/html")){ $text_html = 1; $text_plain = 0; }
            }

            if($state == "CONTENT_TRANSFER_ENCODING"){
               if(preg_match("/quoted-printable/i", $l)){ $qp = 1; }
               if(preg_match("/base64/i", $l)){ $base64 = 1; }
            }


            if($is_header == 1){
               if($l[0] != " " && $l[0] != "\t"){ $state = "UNDEF"; }
               if(preg_match("/^From:/i", $l)){ $state = "FROM"; }
               if(preg_match("/^To:/i", $l) || preg_match("/^Cc:/i", $l)){ $state = "TO"; }
               if(preg_match("/^Date:/i", $l)){ $state = "DATE"; }
               if(preg_match("/^Subject:/i", $l)){ $state = "SUBJECT"; }
               if(preg_match("/^Content-Type:/", $l)){ $state = "CONTENT_TYPE"; }
               if(preg_match("/^Content-Disposition:/", $l)){ $state = "CONTENT_DISPOSITION"; }

               $l = preg_replace("/</", "&lt;", $l);
               $l = preg_replace("/>/", "&gt;", $l);

               if($_1st_header == 1) {
                  if($state == "FROM"){ $from .= preg_replace("/\r|\n/", "", $l); }
                  if($state == "TO"){ $to .= preg_replace("/\r|\n/", "", $l); }
                  if($state == "SUBJECT"){ $subject .= preg_replace("/\r|\n/", "", $l); }
                  if($state == "DATE"){ $date .= preg_replace("/\r|\n/", "", $l); }
               }
            }
            else {

               if($this->check_boundary($boundary, $l) == 1){

                  if($text_plain == 1 || $has_text_plain == 0) {
                     $message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);
                  }

                  $text_plain = $text_html = $qp = $base64 = 0;

                  $charset = $body_chunk = "";

                  $is_header = 1;

                  continue;
               }

               else if(($l[0] == "\r" && $l[1] == "\n") || $l[0] == "\n"){
                  $state = "BODY";
                  $body_chunk .= $l;
               }

               else if($state == "BODY"){
                  if($text_plain == 1 || $text_html == 1){ $body_chunk .= $l; }

               }

            }

         }
      }
 
     fclose($fp);


      if($body_chunk && ($text_plain == 1 || $has_text_plain == 0) ){
         $message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);
      }

      return array('from' => $this->decode_my_str($from),
                   'to' => $this->decode_my_str($to),
                   'subject' => $this->decode_my_str($subject),
                   'date' => $this->decode_my_str($date),
                   'message' => $message
            );

   }


   private function check_boundary($boundary, $line) {

      for($i=0; $i<count($boundary); $i++){
         if(strstr($line, $boundary[$i])){
            return 1;
         }
      }

      return 0;
   }


   private function flush_body_chunk($chunk, $charset, $qp, $base64, $text_plain, $text_html) {

      if($qp == 1){
         $chunk = $this->qp_decode($chunk);
      }

      if($base64 == 1){
         $chunk = base64_decode($chunk);
      }

      if($charset && !preg_match("/utf-8/i", $charset)){
         $s = @iconv($charset, 'utf-8', $chunk);
         if($s) { $chunk = $s; $s = ''; }
      }

      if($text_plain == 1){
         $chunk = preg_replace("/</", "&lt;", $chunk);
         $chunk = preg_replace("/>/", "&gt;", $chunk);

         $chunk = preg_replace("/\n/", "<br />\n", $chunk);
         $chunk = "\n" . $this->print_nicely($chunk);
      }

      if($text_html == 1){
         $chunk = preg_replace("/\<style([^\>]+)\>([\w\W]+)\<\/style\>/i", "", $chunk);

         if(ENABLE_REMOTE_IMAGES == 0) {
            $chunk = preg_replace("/style([\s]{0,}=[\s]{0,})\"([^\"]+)/", "style=\"xxxx", $chunk);
            $chunk = preg_replace("/style([\s]{0,}=[\s]{0,})\'([^\']+)/", "style=\'xxxx", $chunk);
         }

         $chunk = preg_replace("/\<body ([\w\s\;\"\'\#\d\:\-\=]+)\>/i", "<body>", $chunk);

         if(ENABLE_REMOTE_IMAGES == 0) { $chunk = preg_replace("/\<img([^\>]+)\>/i", "<img src=\"" . REMOTE_IMAGE_REPLACEMENT . "\" />", $chunk); }

         /* prevent scripts in the HTML part */

         $chunk = preg_replace("/document\.write/", "document.writeee", $chunk);
         $chunk = preg_replace("/<\s{0,}script([\w\W]+)\/script\s{0,}\>/i", "<!-- disabled javascript here -->", $chunk);
      }

      return $chunk;
   }


   private function flush_body_chunk2($chunk, $charset, $qp, $base64, $text_plain, $text_html) {

      if($qp == 1){
         $chunk = $this->qp_decode($chunk);
      }

      if($base64 == 1){
         $chunk = base64_decode($chunk);
      }

      if(!preg_match("/utf-8/i", $charset)){
         $chunk = utf8_encode($chunk);
      }

      if($text_plain == 1){
         $chunk = preg_replace("/</", "&lt;", $chunk);
         $chunk = preg_replace("/>/", "&gt;", $chunk);

         $chunk = "<pre>\n" . $this->print_nicely($chunk) . "</pre>\n";
      }

      if($text_html == 1){
         $chunk = preg_replace("/\<style([^\>]+)\>([\w\W]+)\<\/style\>/i", "", $chunk);

         if(ENABLE_REMOTE_IMAGES == 0) {
            $chunk = preg_replace("/style([\s]{0,}=[\s]{0,})\"([^\"]+)/", "style=\"xxxx", $chunk);
            $chunk = preg_replace("/style([\s]{0,}=[\s]{0,})\'([^\']+)/", "style=\'xxxx", $chunk);
         }

         $chunk = preg_replace("/\<body ([\w\s\;\"\'\#\d\:\-\=]+)\>/i", "<body>", $chunk);

         if(ENABLE_REMOTE_IMAGES == 0) { $chunk = preg_replace("/\<img([^\>]+)\>/i", "<img src=\"" . REMOTE_IMAGE_REPLACEMENT . "\" />", $chunk); }

         /* prevent scripts in the HTML part */

         $chunk = preg_replace("/document\.write/", "document.writeee", $chunk);
         $chunk = preg_replace("/<\s{0,}script([\w\W]+)\/script\s{0,}\>/i", "<!-- disabled javascript here -->", $chunk);
      }

      return $chunk;
   }


   private function print_nicely($chunk) {
      $k = 0;
      $nice_chunk = "";

      $x = explode(" ", $chunk);

      for($i=0; $i<count($x); $i++){
         $nice_chunk .= "$x[$i] ";
         $k += strlen($x[$i]);

         if(strstr($x[$i], "\n")){ $k = 0; }

         if($k > 70){ $nice_chunk .= "\n"; $k = 0; }
      }

      return $nice_chunk;
   }


   public function NiceSize($size) {
      if($size < 1000) return "1k";
      if($size < 100000) return round($size/1000) . "k";

      return sprintf("%.1f", $size/1000000) . "M";
   }


   private function qp_decode($l) {
      $res = "";
      $c = "";

      if($l == ""){ return ""; }

      /* remove soft breaks at the end of lines */

      if(preg_match("/\=\r\n/", $l)){ $l = preg_replace("/\=\r\n/", "", $l); }
      if(preg_match("/\=\n/", $l)){ $l = preg_replace("/\=\n/", "", $l); }

      for($i=0; $i<strlen($l); $i++){
         $c = $l[$i];

         if($c == '=' && ctype_xdigit($l[$i+1]) && ctype_xdigit($l[$i+2])){
            $a = $l[$i+1];
            $b = $l[$i+2];

            $c = chr(16*hexdec($a) + hexdec($b));

            $i += 2;
         }

         $res .= $c;

      }

      return $res;
   }


   public function scan_message($dir, $f) {
      $language = Registry::get('language');

      $from = $subject = "";
      $is_header=1;
      $state = "UNDEF";

      if(!is_readable($dir . "/" . $f)) { return array ($from, $subject); }

      $fp = fopen($dir . "/" . $f, "r");
      if($fp){

          while(($l = fgets($fp, 4096))){
            if(($l[0] == "\r" && $l[1] == "\n" && $is_header == 1) || ($l[0] == "\n" && $is_header == 1) ){
               break;
            }

            if($is_header == 1){
               if($l[0] != " " && $l[0] != "\t"){ $state = "UNDEF"; }
               if(preg_match("/^From:/", $l)){ $state = "FROM"; }
               if(preg_match("/^To:/", $l)){ $state = "TO"; }
               if(preg_match("/^Date:/", $l)){ $state = "DATE"; }
               if(preg_match("/^Subject:/", $l)){ $state = "SUBJECT"; }
               if(preg_match("/^Content-Type:/", $l)){ $state = "CONTENT_TYPE"; }
            }

            if($state == "SUBJECT"){
               $l = preg_replace("/^\s{1,}/", "", $l);
               $subject .= $l;
            }
            if($state == "FROM"){
               $l = preg_replace("/^\s{1,}/", "", $l);
               $from .= $l;
            }
         }

         fclose($fp);
      }

      $from = preg_replace("/^From\:\s{0,}/", "", $from);
      if($from) { $from = htmlspecialchars($this->decode_my_str($from)); }
      else { $from = $language->get('text_no_sender'); }

      $subject = preg_replace("/^Subject\:\s{0,}/", "", $subject);
      if($subject) { $subject = htmlspecialchars($this->decode_my_str($subject)); }
      else { $subject = $language->get('text_no_subject'); }

      return array ($from, $subject);
   }


   public function decode_my_str($what = '') {
      $result = "";

      $what = rtrim($what);

      $what = preg_replace("/\s/", " ", $what);
      $a = preg_split("/\s/", $what);

      while(list($k, $v) = each($a)){
         $result .= ' ';

         $str1 = preg_split("/\?\=/", $v);

         while(list($k2, $v2) = each($str1)) {

            if(substr($v2, 0, 2) == "=?") {
               $result .= $this->fix_encoded_string($v2 . "?=");
            } else {
               $result .= $v2;
            }
         }

      }

      return $result;
   }


   private function fix_encoded_string($what = '') {
      $s = "";

      $what = rtrim($what, "\"\r\n");
      $what = ltrim($what, "\"");

      if(preg_match("/^\=\?/", $what) && preg_match("/\?\=$/", $what)){
         $what = preg_replace("/^\=\?/", "", $what);
         $what = preg_replace("/\?\=$/", "", $what);

         $encoding = substr($what, 0, strpos($what, '?'));

         if(preg_match("/\?Q\?/i", $what)){
            $x = preg_replace("/^([\w\-]+)\?Q\?/i", "", $what);

            $s = quoted_printable_decode($x);
            $s = preg_replace("/_/", " ", $s);
         }

         if(preg_match("/\?B\?/i", $what)){
            $x = preg_replace("/^([\w\-]+)\?B\?/i", "", $what);

            $s = base64_decode($x);
            $s = preg_replace('/\0/', "*", $s);
         }

         if(!preg_match("/utf-8/i", $encoding)){
            $s2 = iconv($encoding, 'utf-8', $s);
            if($s2) { $s = $s2; }
         }

      }
      else {
         $s = utf8_encode($what);
      }

      return $s;
   }


}

?>
