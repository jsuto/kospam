<?php

class ModelQuarantineMessage extends Model {


   public function myscandir($dir, $exp = '', $desc=0, $ts = 0) {
      $r = array();

      $dh = @opendir($dir);
      if($dh) {
         while(($fname = readdir($dh)) !== false) {
            if($exp == "" || preg_match($exp, $fname)) {
               $stat = stat("$dir/$fname");
               if( ($desc == 1 && $stat['ctime'] > $ts) || $desc == 0) {
                  $r[$fname] = $stat['ctime'];
               }
            }
         }
         closedir($dh);

         if($desc) {
            arsort($r);
         }
         else {
            asort($r);
         }

      }

      return(array_keys($r));
   }


   public function checkId($id = '') {
      if(strlen($id) > 10 && preg_match('/^([sh]\.[0-9a-f]+)$/', $id)){
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
                  $message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);

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
                  if($n_image >= 0){ $images[$n_image] .= rtrim($l); }
               }

            }


         }
         fclose($fp);
      }

      if($body_chunk){
         $message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);
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

            $message .= "\n<p><img src=\"" . WEBUI_DIRECTORY . "/cache/$id-$n\" alt=\"spam image\" /></p>\n";

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


   private function flush_body_chunk($chunk, $charset, $qp, $base64, $text_plain, $text_html) {

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

      $from = $language->get('text_no_sender');
      $subj = $language->get('text_no_subject');


      $is_header=1;
      $state = "UNDEF";

      if(!is_readable($dir . "/" . $f)) { return array ($from, $subj); }

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
               $subj .= $this->decode_my_str($l);
            }
            if($state == "FROM"){
               $l = preg_replace("/^\s{1,}/", "", $l);
               $from .= $this->decode_my_str($l);
            }
         }

         $from = substr($from, strlen($language->get('text_no_sender'))+5, 4096);
         $subj = substr($subj, strlen($language->get('text_no_subject'))+8, 4096);

         $from = preg_replace("/^\s{1,}/", "", $from);
         $subj = preg_replace("/^\s{1,}/", "", $subj);

         fclose($fp);
      }

      $from = preg_replace("/</", "&lt;", $from);
      $from = preg_replace("/>/", "&gt;", $from);

      $subj = preg_replace("/</", "&lt;", $subj);
      $subj = preg_replace("/>/", "&gt;", $subj);

      $from = preg_replace("/'/", '"', $from);
      $subj = preg_replace("/'/", '"', $subj);

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

         if(!preg_match("/utf-8/i", $what)){
            $s = utf8_encode($s);
         }

      }
      else {
         $s = utf8_encode($what);
      }

      return $s;
   }


}

?>
