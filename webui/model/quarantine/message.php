<?php

class ModelQuarantineMessage extends Model {

   public function checkId($id = '') {
      if(strlen($id) > 10 && preg_match('/^(s\.[0-9a-f]+)$/', $id)){
         return 1;
      }

      return 0;
   }


   public function getMessages($dir = '', $username = '', $page = 0, $page_len = PAGE_LEN, $from = '', $subj = '') {
      $n_spam = $n_msgs = $spam_total_size = 0;
      $messages = array();

      if($dir == "" || !file_exists($dir)) { return array($n_msgs, $spam_total_size, $messages); }


      $files = scandir($dir, 1);

      while(list($k, $v) = each($files)){
         if(strncmp($v, "s.", 2) == 0 && $this->checkId($v)){

            $f = $dir . "/" . $v;

            list ($mailfrom, $subject) = $this->scan_message($dir, $v);

            if($this->is_it_in($subject, $subj) && $this->is_it_in($mailfrom, $from)){

               $n_spam++;

               if(($st = stat($f))){ $spam_total_size += $st['size']; }

               if($n_spam > $page_len*$page && $n_spam <= $page_len*($page+1)){

                  $messages[] = array(
                                       'i' => $n_spam,
                                       'id' => $v,
                                       'from' => $mailfrom,
                                       'subject' => $subject,
                                       'date' => date("Y.m.d.", $st['mtime'])
                                     );

               }
            }
         }
      }


      return array($n_spam, $spam_total_size, $messages);
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

      $message = preg_replace("/</", "&lt;", $message);
      $message = preg_replace("/>/", "&gt;", $message);

      return "<pre>\n" . $this->print_nicely($message) . "</pre>\n";
   }


   public function ShowMessage($dir = '', $id = '') {
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
      $images = array();
      $n_image = -1;

      $message = "";

      if($dir == "" || $id == "" || !$this->checkId($id) || !file_exists($dir . "/" . $id) || !is_readable($dir . "/" . $id) ){ return ""; }

      $fp = fopen($dir . "/" . $id, "r");
      if($fp){
         while(($l = fgets($fp, 4096))){
            if(($l[0] == "\r" && $l[1] == "\n" && $is_header == 1) || ($l[0] == "\n" && $is_header == 1) ){
               $message .= "<pre>$header</pre>\n\n";
               $is_header = 0;
            }

            if(preg_match("/^Content-Type:/i", $l)) $state = "CONTENT_TYPE";
            if(preg_match("/^Content-Transfer-Encoding:/i", $l)) $state = "CONTENT_TRANSFER_ENCODING";

            if($state == "CONTENT_TYPE"){
               $x = strstr($l, "boundary");
               if($x){
                  $x = preg_replace("/\"/", "", $x);
                  $x = preg_replace("/\'/", "", $x);
                  //$x = preg_replace("/ /", "", $x);

                  $b = explode("boundary=", $x);
                  array_push($boundary, rtrim($b[1]));
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

               if(strstr($l, "text/plain")){ $text_plain = 1; }
               if(strstr($l, "text/html")){ $text_html = 1; $text_plain = 0; }

               if(strstr($l, "image/jpeg") || strstr($l, "image/png") || strstr($l, "image/gif") ){
                  array_push($images, "");
                  $n_image++;
               }

            }

            if($state == "CONTENT_TRANSFER_ENCODING"){
               if(strstr($l, "quoted-printable")){ $qp = 1; }
               if(strstr($l, "base64")){ $base64 = 1; }
            }


            if($is_header == 1){
               if($l[0] != " " && $l[0] != "\t"){ $state = "UNDEF"; }
               if(preg_match("/^From:/", $l)){ $state = "FROM"; }
               if(preg_match("/^To:/", $l)){ $state = "TO"; }
               if(preg_match("/^Date:/", $l)){ $state = "DATE"; }
               if(preg_match("/^Subject:/", $l)){ $state = "SUBJECT"; }
               if(preg_match("/^Content-Type:/", $l)){ $state = "CONTENT_TYPE"; }

               $l = preg_replace("/</", "&lt;", $l);
               $l = preg_replace("/>/", "&gt;", $l);

               if($state == "FROM" || $state == "TO" || $state == "SUBJECT" || $state == "DATE") {
                  $header .= $this->decode_my_str($l) . "\r\n";
               }

            }
            else {

               if($this->check_boundary($boundary, $l) == 1){
                  $message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $text_plain, $text_html);

                  $text_plain = $text_html = $qp = $base64 = 0;

                  $charset = $body_chunk = "";

                  continue;
               }

               else if(($l[0] == "\r" && $l[1] == "\n") || $l[0] == "\n"){
                  $state = "BODY";
                  $body_chunk .= $l;
               }

               else if($state == "BODY"){
                  if($text_plain == 1 || $text_html == 1){ $body_chunk .= $l; }

               }

               if($state == "BODY" && $base64 == 1 && !preg_match("/^[\r\n]/", $l) ) {
                  $images[$n_image] .= rtrim($l);
               }

            }


         }
         fclose($fp);
      }

      if($body_chunk){
         $message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $text_plain, $text_html);
      }

      /* write images to the cache dir, if possible */

      if(file_exists(DIR_CACHE)) {
         $n = 0;

         foreach ($images as $image) {
            $f = DIR_CACHE . "$id-$n";

            if(!file_exists($f)) {
               $fp = fopen($f, "w+");
               if($fp){
                  fputs($fp, base64_decode($image));
                  fclose($fp);
               }
            }

            $message .= "\n<p><img src=\"/cache/$id-$n\" alt=\"spam image\" /></p>\n";

            $n++;
         }
      }


      return $message;
   }


   private function check_boundary($boundary, $line) {

      for($i=0; $i<count($boundary); $i++){
         if(strstr($line, $boundary[$i])){
            return 1;
         }
      }

   return 0;
}


   private function flush_body_chunk($chunk, $charset, $qp, $text_plain, $text_html) {
      if($qp == 1){
         $chunk = $this->qp_decode($chunk);
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
         $chunk = preg_replace("/\<style\>([\w\W]+)\<\/style\>/i", "", $chunk);
         $chunk = preg_replace("/\<body ([\w\s\;\"\'\#\d\:\-\=]+)\>/i", "<body>", $chunk);

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


   private function scan_message($dir, $f) {
      $language = Registry::get('language');

      $from = $language->get('text_no_sender');
      $subj = $language->get('text_no_subject');

      $i = 0;

      if(!is_readable($dir . "/" . $f)) { return array ($from, $subj); }

      $fp = fopen($dir . "/" . $f, "r");
      if($fp){
         while(($l = fgets($fp, 4096))){
            if(strncmp($l, "Subject:", 8) == 0 && strlen($l) > 10){
               $subj = substr($l, 9, 4096);
               $subj = $this->fix_encoded_string($subj);

               $len = strlen($subj);

               if(strlen($subj) > 9+MAX_CGI_FROM_SUBJ_LEN){
                  $subj = substr($subj, 0, MAX_CGI_FROM_SUBJ_LEN) . "...";
               }

               $i++;
            }

            if(strncmp($l, "From:", 5) == 0 && strlen($l) > 10){
               $from = substr($l, 6, 4096);
               $from = $this->decode_my_str($from);
               if(strlen($from) > 6+MAX_CGI_FROM_SUBJ_LEN){
                  $from = substr($from, 0, MAX_CGI_FROM_SUBJ_LEN) . "...";
               }
               $i++;
            }

            if($i >= 2){
               break;
            }
         }

         fclose($fp);
      }

      $from = preg_replace("/</", "&lt;", $from);
      $from = preg_replace("/>/", "&gt;", $from);


      return array ($from, $subj);
   }


   private function decode_my_str($what = '') {
      $result = "";

      $what = rtrim($what);

      $a = explode(" ", $what);

      while(list($k, $v) = each($a)){
         if($k > 0){
            $result .= " ";
         }

         $result .= $this->fix_encoded_string($v);
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

         if(preg_match("/\?Q\?/", $what)){
            list($enc, $x) = explode("?Q?", $what);

            $s = quoted_printable_decode($x);
            $s = preg_replace("/_/", " ", $s);
         }

         if(preg_match("/\?B\?/", $what)){
            list($enc, $x) = explode("?B?", $what);

            $s = base64_decode($x);
         }

      }
      else {
         $s = utf8_encode($what);
      }

      return $s;
   }


   private function is_it_in($in, $what) {
      if($in == ""){ return 0; }

      if($what == ""){ return 1; }

      if(stristr($in, $what)){ return 1; }

      return 0;
   }

}

?>
