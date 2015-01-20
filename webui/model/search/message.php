<?php

class ModelSearchMessage extends Model {

   public $encoding_aliases = array(
                                     'GB2312'   => 'GBK',
                                     'GB231280' => 'GBK'
                                   );


   public function get_filename_by_clapf_id($id = '') {
      if($id == '' || !preg_match("/^([0-9a-f]+)$/", $id)) { return ""; }

      $d2 = substr($id, 36-2, 2);
      $d1 = substr($id, 36-4, 2);

      $server_id = substr($id, 36-12, 2);

      return DIR_QUEUE . $server_id . '/' . $d1 . '/' . $d2 . '/' . $id;
   }


   public function get_raw_message($id = '') {
      $s = '';

      $filename = $this->get_filename_by_clapf_id($id);

      if($filename == '') { return $s; }

      $fp = fopen($filename, "r");
      if($fp){
         while(($buf = fgets($fp, 8192))) {
            $s .= $buf;
         }
      }
      fclose($fp);

      return $s;
   }


   public function get_message_headers($id = '') {
      $data = '';

      $msg = $this->get_raw_message($id);

      $pos = strpos($msg, "\n\r\n");
      if($pos == false) {
         $pos = strpos($msg, "\n\n");
      }

      if($pos == false) { return $msg; }

      $data = substr($msg, 0, $pos);
      $msg = '';

      $data = preg_replace("/\</", "&lt;", $data);
      $data = preg_replace("/\>/", "&gt;", $data);

      return array('headers' => $data);
   }


   public function extract_message($id = '', $terms = '') {
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
      $verification = 1;

      $from = $to = $subject = $date = $text_message = $html_message = "";

      $msg = $this->get_raw_message($id);

      $a = explode("\n", $msg); $msg = "";

      while(list($k, $l) = each($a)){
            $l .= "\n";

            if(($l[0] == "\r" && $l[1] == "\n" && $is_header == 1) || ($l[0] == "\n" && $is_header == 1) ){
               $is_header = $_1st_header = 0;

               if($rfc822 == 1) { $rfc822 = 0; $is_header = 1; }

            }

            if($is_header == 1 && preg_match("/^Content-Type:/i", $l)) $state = "CONTENT_TYPE";
            if($is_header == 1 && preg_match("/^Content-Transfer-Encoding:/i", $l)) $state = "CONTENT_TRANSFER_ENCODING";

            if($state == "CONTENT_TYPE"){

               $x = stristr($l, "boundary");
               if($x){

                  $s1 = explode(";", $x);
                  $x = $s1[0];

                  $x = preg_replace("/boundary\s{0,}=\s{0,}/i", "boundary=", $x);
                  //$x = preg_replace("/boundary= /i", "boundary=", $x);

                  $x = preg_replace("/\"\;{0,1}/", "", $x);
                  $x = preg_replace("/\'/", "", $x);

                  $b = explode("boundary=", $x);

                  $__boundary = rtrim($b[count($b)-1]);

                  if($__boundary) { array_push($boundary, $__boundary); }

               }

               if(preg_match("/charset/i", $l)){
                  $types = explode(";", $l);
                  foreach ($types as $type){
                     if(preg_match("/charset/i", $type)){
                        $type = preg_replace("/[\"\'\ ]/", "", $type);

                        $x = explode("=", $type);
                        $charset = rtrim(strtoupper($x[1]));

                        if(isset($this->encoding_aliases[$charset])) { $charset = $this->encoding_aliases[$charset]; }

                     }
                  }
               }

               if(strstr($l, "message/rfc822")) { $rfc822 = 1; }

               if(stristr($l, "text/plain")){ $text_plain = 1; $text_html = 0; $has_text_plain = 1; }
               if(stristr($l, "text/html")){ $text_html = 1; $text_plain = 0; }
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
                     $text_message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);
                  }

                  if($text_html == 1) {
                     $html_message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);
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

      if($body_chunk) {
         if($text_plain == 1 || $has_text_plain == 0) {
            $text_message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);
         }

         if($text_html == 1) {
            $html_message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);
         }
      }


      if(strlen($html_message) > 20) {
         $message = $this->highlight_search_terms($html_message, $terms, 1);
      } else {
         $message = $this->highlight_search_terms($text_message, $terms);
      }

      return array('from' => $this->decode_my_str($from),
                   'to' => $this->decode_my_str($to),
                   'subject' => $this->highlight_search_terms($this->decode_my_str($subject), $terms),
                   'date' => $this->decode_my_str($date),
                   'message' => $message,
                   'verification' => $verification
            );
   }


   private function highlight_search_terms($s = '', $terms = '', $html = 0) {
      $fields = array("from:", "to:", "subject:", "body:");

      $terms = preg_replace("/(\'|\"|\=|\>|\<)/", "", $terms);

      $a = explode(" ", $terms);
      $terms = array();

      while(list($k, $v) = each($a)) {
         if(strlen($v) >= 3 && !in_array($v, $fields)) {
            //$v = preg_replace("/\W/", "", $v);
            if($v) { array_push($terms, $v); }
         }
      }

      if(count($terms) <= 0) { return $s; }


      if($html == 0) {
         while(list($k, $v) = each($terms)) {
            $s = preg_replace("/$v/i", "<span class=\"message_highlight\">$v</span>", $s);
         }

         return $s;
      }


      $tokens = preg_split("/\</", $s);
      $s = '';

      while(list($k, $token) = each($tokens)) {

         $pos = strpos($token, ">");
         if($pos > 0) {
            $len = strlen($token);

            $s .= '<' . substr($token, 0, $pos) . '>';

            if($len > $pos+1) {
               $str = substr($token, $pos+1, $len);

               reset($terms);
               while(list($k, $v) = each($terms)) {
                  $str = preg_replace("/$v/i", "<span class=\"message_highlight\">$v</span>", $str);
               }

               $s .= $str;
            }

         }
      }

      return $s;
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
         $s = @iconv($charset, 'utf-8' . '//IGNORE', $chunk);
         if($s) { $chunk = $s; $s = ''; }
      }

      if($text_plain == 1){
         $chunk = preg_replace("/</", "&lt;", $chunk);
         $chunk = preg_replace("/>/", "&gt;", $chunk);

         $chunk = preg_replace("/\n/", "<br />\n", $chunk);
         $chunk = "\n" . $this->print_nicely($chunk);
      }

      if($text_html == 1){

         $h = preg_split("/\<style/i", $chunk);
         $chunk = '';

         for($i=0; $i<count($h); $i++) {
            $pos = stripos($h[$i], "</style>");
            if($pos != FALSE) {
               $s = substr($h[$i], $pos+8, strlen($h[$i]));
               $chunk .= $s . "\n";
            }
            else { $chunk .= $h[$i] . "\n"; }
         }


         if(ENABLE_REMOTE_IMAGES == 0) {
            $chunk = preg_replace("/style([\s]{0,}=[\s]{0,})\"([^\"]+)/", "style=\"xxxx", $chunk);
            $chunk = preg_replace("/style([\s]{0,}=[\s]{0,})\'([^\']+)/", "style='xxxx", $chunk);
         }

         $chunk = preg_replace("/\<body ([\w\s\;\"\'\#\d\:\-\=]+)\>/i", "<body>", $chunk);

         if(ENABLE_REMOTE_IMAGES == 0) { $chunk = preg_replace("/\<img([^\>]+)\>/i", "<img src=\"" . REMOTE_IMAGE_REPLACEMENT . "\" />", $chunk); }

         /* prevent scripts in the HTML part */

         $chunk = preg_replace("/document\.write/", "document.writeee", $chunk);
         $chunk = preg_replace("/<\s{0,}script([\w\W]+)\/script\s{0,}\>/i", "<!-- disabled javascript here -->", $chunk);

         $chunk = preg_replace("/\<base\s{1,}href/", "<bbbase href", $chunk);
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


   public function decode_my_str($what = '') {
      $result = "";

      $what = rtrim($what);

      $a = preg_split("/\s/", $what);

      while(list($k, $v) = each($a)){
         $x = preg_match("/\?\=$/", $v);

         if( ($x == 0 && $k > 0) || ($x == 1 && $k == 1) ){
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

         $encoding = strtoupper(substr($what, 0, strpos($what, '?')));
         if(isset($this->encoding_aliases[$encoding])) { $encoding = $this->encoding_aliases[$encoding]; }

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
            $s = iconv($encoding, 'utf-8' . '//IGNORE', $s);
         }

      }
      else {
         $s = utf8_encode($what);
      }

      return $s;
   }


   public function get_clapf_id_by_id($id = 0) {
      $query = $this->db->query("SELECT `clapf_id` FROM `" . TABLE_HISTORY . "` WHERE id=?", array($id));
      if(isset($query->row['clapf_id'])) { return $query->row['clapf_id']; }
      return '';
   }


   public function get_rcpt_by_id($id = 0) {
      $query = $this->db->query("SELECT `rcpt` FROM `" . TABLE_HISTORY . "` WHERE id=?", array($id));
      if(isset($query->row['rcpt'])) { return $query->row['rcpt']; }
      return '';
   }


   public function is_message_spam($id = 0) {
      $spam = 0;

      $query = $this->db->query("SELECT spam FROM " . TABLE_HISTORY . " WHERE id=?", array($id));

      if(isset($query->row['spam'])) { $spam = $query->row['spam']; }

      return $spam;
   }


}

?>
