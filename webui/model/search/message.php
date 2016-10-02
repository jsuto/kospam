<?php

require 'Zend/Mime/Decode.php';

class ModelSearchMessage extends Model {

   public $encoding_aliases = array(
                                     'GB2312'   => 'GBK',
                                     'GB231280' => 'GBK'
                                   );

   public $message;


   public function get_boundary($line='') {
      $parts = explode(";", $line);

      for($i=0; $i<count($parts); $i++) {
         if(stristr($parts[$i], "boundary")) {
            $parts[$i] = preg_replace("/boundary\s{0,}=\s{0,}/i", "boundary=", $parts[$i]);
            $parts[$i] = preg_replace("/\"\;{0,1}/", "", $parts[$i]);
            $parts[$i] = preg_replace("/\'/", "", $parts[$i]);

            $b = explode("boundary=", $parts[$i]);

            return rtrim($b[count($b)-1]);
         }
      }

      return "";
   }


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

      Zend_Mime_Decode::splitMessageRaw($msg, $headers, $body);

      $headers = $this->escape_lt_gt_symbols($headers);

      return array('headers' => $headers);
   }


   public function extract_message($id = '', $terms = '') {
      $from = "From: ";
      $to = "To: ";
      $subject = "Subject: ";
      $date = "Date: ";
      $verification = 1;

      $msg = $this->get_raw_message($id);


      Zend_Mime_Decode::splitMessage($msg, $headers, $body);
      $boundary = $this->get_boundary($headers['content-type']);

      if(is_array($headers['date'])) { $headers['date'] = $headers['date'][0]; }

      if(isset($headers['from'])) $from .= $this->escape_lt_gt_symbols($headers['from']);
      if(isset($headers['to'])) $to .= $this->escape_lt_gt_symbols($headers['to']);
      if(isset($headers['subject'])) $subject .= $this->escape_lt_gt_symbols($headers['subject']);
      if(isset($headers['date'])) $date .= $headers['date'];

      $this->message = array(
                             'text/plain' => '',
                             'text/html' => ''
                            );

      $this->extract_textuals_from_mime_parts($headers, $body, $boundary);

      return array('from' => $from,
                   'to' => $to,
                   'subject' => $this->highlight_search_terms($subject, $terms),
                   'date' => $date,
                   'message' => $this->message['text/html'] ? $this->message['text/html'] : $this->message['text/plain'],
                   'verification' => $verification
            );
   }


   private function extract_textuals_from_mime_parts($headers = array(), $body = '', $boundary = '') {
      $mime_parts = array();

      if($boundary) {
         $mime_parts = Zend_Mime_Decode::splitMessageStruct($body, $boundary);
      } else {
         $mime_parts[] = array('header' => $headers, 'body' => $body);
      }

      for($i=0; $i<count($mime_parts); $i++) {
         $mime = array(
                       'content-type' => '',
                       'encoding' => ''
                      );

         if(isset($mime_parts[$i]['header']['content-type']))
            $mime['content-type'] = Zend_Mime_Decode::splitContentType($mime_parts[$i]['header']['content-type']);

         if(in_array($mime['content-type']['type'], array('multipart/mixed', 'multipart/related', 'multipart/alternative')))
            $this->extract_textuals_from_mime_parts($mime_parts[$i]['header'], $mime_parts[$i]['body'], $mime['content-type']['boundary']);

         if(isset($mime_parts[$i]['header']['content-transfer-encoding']))
            $mime['encoding'] = $mime_parts[$i]['header']['content-transfer-encoding'];

         if(in_array($mime['content-type']['type'], array('text/plain', 'text/html')))
            $this->message[$mime['content-type']['type']] .= $this->fix_mime_body_part($mime, $mime_parts[$i]['body']);
      }
   }


   private function fix_mime_body_part($mime = array(), $body = '') {
      if($mime['encoding'] == 'quoted-printable')
         $body = Zend_Mime_Decode::decodeQuotedPrintable($body);

      if($mime['encoding'] == 'base64')
         $body = base64_decode($body);

      if(strtolower($mime['content-type']['charset']) != 'utf-8')
         $body = iconv($mime['content-type']['charset'], 'utf-8' . '//IGNORE', $body);


      if(strtolower($mime['content-type']['type']) == 'text/plain') {

         $body = $this->escape_lt_gt_symbols($body);

         $body = preg_replace("/\n/", "<br />\n", $body);
         $body = "\n" . $this->print_nicely($body);
      }

      if(strtolower($mime['content-type']['type']) == 'text/html') {

         $body = preg_replace("/\<style([\w\W]+)style\>/", "", $body);

         if(ENABLE_REMOTE_IMAGES == 0) {
            $body = preg_replace("/style([\s]{0,}=[\s]{0,})\"([^\"]+)/", "style=\"xxxx", $body);
            $body = preg_replace("/style([\s]{0,}=[\s]{0,})\'([^\']+)/", "style='xxxx", $body);

            $body = preg_replace("/\<img([^\>]+)\>/i", "<img src=\"" . REMOTE_IMAGE_REPLACEMENT . "\" />", $body);
         }

         $body = preg_replace("/\<body ([\w\s\;\"\'\#\d\:\-\=]+)\>/i", "<body>", $body);

         $body = preg_replace("/\<a\s{1,}([\w=\"\'\s]+){0,}\s{0,}href/i", "<qqqq", $body);
         $body = preg_replace("/\<base href/i", "<qqqq", $body);

         $body = preg_replace("/document\.write/", "document.writeee", $body);
         $body = preg_replace("/<\s{0,}script([\w\W]+)\/script\s{0,}\>/i", "<!-- disabled javascript here -->", $body);
      }

      return $body;
   }


   private function highlight_search_terms($s = '', $terms = '', $html = 0) {
      $fields = array("from:", "to:", "subject:", "body:");

      //$terms = preg_replace("/(\'|\"|\=|\>|\<)/", "", $terms);
      $terms = preg_replace("/(\,|\s){1,}/", " ", $terms);

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


   private function escape_lt_gt_symbols($s = '') {
      $s = preg_replace("/</", "&lt;", $s);
      $s = preg_replace("/>/", "&gt;", $s);

      return $s;
   }


   private function print_nicely($chunk) {
      $k = 0;
      $nice_chunk = "";

      $x = explode(" ", $chunk);

      for($i=0; $i<count($x); $i++){
         $nice_chunk .= $x[$i] . " ";
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
