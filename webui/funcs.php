<?php


function errout($msg){
   print $msg;

   exit(1);
}


function nice_error($msg){
   global $ERROR;

   include_once("header.php");

   print "<h3>$ERROR</h3>\n";

   print "<div id=\"body\">\n\n\n";

   print "<p>$msg</p>\n\n";

   print "</div> <!-- body -->\n\n\n";

   include_once("footer.php");

   exit(1);
}


function nice_screen($msg){
   include_once("header.php");

   print "<h3>OK</h3>\n";

   print "<div id=\"body\">\n\n\n";

   print "<p>$msg</p>\n\n";

   print "</div> <!-- body -->\n\n\n";

   include_once("footer.php");

   exit(1);
}


function make_seed(){
   list($usec, $sec) = explode(' ', microtime());
   return (float) $sec + ((float) $usec * 100000);
}


function gen_random_id($n){
   $LETTERS="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
   $len = strlen($LETTERS);
   $x = "";

   for($i=0; $i<$n; $i++){
      $a = rand(1, 2048) % $len;
      $x .= substr($LETTERS, $a, 1);
   }

   return $x;
}


function trim_to_array($x){
   $a = array();

   $z = explode("\n", $x);
   while(list($k, $v) = each($z)){
      $v = rtrim($v);
      if($v) array_push($a, $v);
   }

   if(count($a) == 0) array_push($a, "");

   return $a;
}


function show_yes_or_no($id = 0){
   $a = array();
   $a[0] = 0;
   $a[1] = 1;

   for($i=0; $i<2; $i++){
      if($i == $id)
         print "<option value=\"$i\" selected>$a[$i]</option>\n";
      else
         print "<option value=\"$i\">$a[$i]</option>\n";
   }

}


function check_email($email){
   if($email == "")
      return false;

   list($user, $domain) = explode("@", $email);
   if(!$user || !$domain) return false;

   if(!checkdnsrr($domain, "MX") && !checkdnsrr($domain, "A")) return false;

   if(preg_match("/^([a-zA-Z0-9-._]+)@([a-zA-Z0-9.-_]+)([a-zA-Z]+)$/", $email))
      return true;

   return false;
}


function send_smtp_email($smtphost, $smtpport, $yourdomain, $from, $to, $msg){
   $ok = 0;

   if($to == "") return $ok;

   $r = fsockopen($smtphost, $smtpport);
   if(!$r) return -1;

   $l = fgets($r, 4096);

   fputs($r, "HELO $yourdomain\r\n");
   $l = fgets($r, 4096);

   fputs($r, "MAIL FROM: <$from>\r\n");
   $l = fgets($r, 4096);

   fputs($r, "RCPT TO: <$to>\r\n");
   $l = fgets($r, 4096);


   fputs($r, "DATA\r\n");
   $l = fgets($r, 4096);
   if(!preg_match("/^354/", $l)) $l = fgets($r, 4096);

   fputs($r, $msg . "\r\n.\r\n");
   $l = fgets($r, 4096);

   if(preg_match("/^250/", $l)){ $ok = 1; }

   fputs($r, "QUIT\r\n");
   $l = fgets($r, 4096);

   fclose($r);

   return $ok;
}


function is_it_in($in, $what){
   if($in == "") return 0;

   if($what == "") return 1;

   if(stristr($in, $what)) return 1;
   else return 0;
}


function fix_encoded_string($what){
   $s = "";

   $what = rtrim($what, "\"\r\n");
   $what = ltrim($what, "\"");

   // From: =?UTF-8?B?S8O2emjDoWzDsyBJSSAtIEhQU0Q=?= <servicedesk@netvisor.hu>

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

      if(preg_match("/utf-8/i", $enc)) $s = utf8_decode($s);

   }
   else
      $s = $what;

   return $s;
}


function decode_my_str($what){
   $result = "";

   $what = rtrim($what);

   $a = explode(" ", $what);
   while(list($k, $v) = each($a)){
      if($k > 0) $result .= " ";
      $result .= fix_encoded_string($v);
   }

   return $result;
}


function scan_message($dir, $f){
   global $NO_SENDER, $NO_SUBJECT, $max_cgi_from_subj_len;

   $from = $NO_SENDER;
   $subj = $NO_SUBJECT;

   $i = 0;


   $fp = fopen($dir . "/" . $f, "r");
   if($fp){
      while(($l = fgets($fp, 4096))){
         if(strncmp($l, "Subject:", 8) == 0 && strlen($l) > 10){
            $subj = substr($l, 9, 4096);
            $subj = fix_encoded_string($subj);

            $len = strlen($subj);

            if(strlen($subj) > 9+$max_cgi_from_subj_len) $subj = substr($subj, 0, $max_cgi_from_subj_len) . "...";
            $i++;
         }
         if(strncmp($l, "From:", 5) == 0 && strlen($l) > 10){
            $from = substr($l, 6, 4096);
            $from = decode_my_str($from);
            if(strlen($from) > 6+$max_cgi_from_subj_len) $from = substr($from, 0, $max_cgi_from_subj_len) . "...";
            $i++;
         }

         if($i >= 2)
            break;
      }
      fclose($fp);
   }

   $from = preg_replace("/</", "&lt;", $from);
   $from = preg_replace("/>/", "&gt;", $from);


   return array ($from, $subj);
}


function get_message_for_delivery($f, $new_from){
   $msg = "";

   $fp = fopen($f, "r");
   if($fp){
      while(($l = fgets($fp, 4096))){
         /*if(strncmp($l, "From:", 5) == 0)
            $msg .= "From: $new_from\r\n";         
         else
            $msg .= $l;*/

         /* do not modify the From: line, 2008.11.21, SJ */
         $msg .= $l;

      }
      fclose($fp);
   }

   return $msg;
}


function check_directory($dir, $username, $page, $from, $subj){
   global $page_len, $max_cgi_from_subj_len, $NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE, $DATE, $FROM, $SUBJECT, $PURGE_SELECTED_MESSAGES, $PURGE_ALL_MESSAGES_FROM_QUARANTINE, $CANCEL, $SELECT_ALL;
   global $DELIVER_SELECTED_MESSAGES, $DELIVER_AND_TRAIN_SELECTED_MESSAGES;

   $n_spam = 0;
   $spam_total_size = 0;
   $n_msgs = 0;
   $this_page = array();
   $this_from = array();
   $this_subj = array();

   if($dir == "") return 0;

   $meurl = $_SERVER['PHP_SELF'];

   $files = scandir($dir, 1);

   while(list($k, $v) = each($files)){
      if(strncmp($v, "s.", 2) == 0){
         $n_spam++;

         $f = $dir . "/" . $v;

         if(($st = stat($f))){
            $spam_total_size += $st['size'];
         }

         list ($mailfrom, $subject) = scan_message($dir, $v);

         if(is_it_in($subject, $subj) && is_it_in($mailfrom, $from)){

            $n_msgs++;

            if($n_msgs > $page_len*$page && $n_msgs <= $page_len*($page+1)){
               array_push($this_page, $v);
               array_push($this_from, $mailfrom);
               array_push($this_subj, $subject);

               //print "$f<br>\n";
            }
         }
      }
   }


   print "<form action=\"$meurl\" name=\"aaa1\" method=\"post\">\n";
   print "<input type=\"hidden\" name=\"topurge\" value=\"1\">\n";
   print "<input type=\"hidden\" name=\"user\" value=\"$username\">\n";

   print "<p>$NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE: $n_spam ($spam_total_size bytes)</p>\n";

   $_50_space = "<font color=\"#ffffff\">" . str_repeat("x", 3+$max_cgi_from_subj_len) . "</font>";

   print "<p><table border=\"0\">\n";
   print "<tr align=\"middle\"><th>&nbsp;</th><th>$DATE</th><th>$FROM</th><th>$SUBJECT</th><th>&nbsp;</th></tr>\n";
   print "<tr><td>&nbsp;</td><td>&nbsp;</td><td>$_50_space</td><td>$_50_space</td><td>&nbsp;</td></tr>\n";

   while(list($k, $v) = each($this_page)){
      $K = ($page*$page_len) + $k + 1;
      $date = "YYYY.MM.DD";

      $__v = substr($v, 2, strlen($v));

      if(($st = stat($dir . "/" . $v)))
         //$date = date("Y.m.d. H:i:s", $st['mtime']);
         $date = date("Y.m.d.", $st['mtime']);


      if(($k % 2) == 0)
         print "<tr valign=\"top\">\n<td><a href=\"$meurl?id=$v&user=$username\">$K.</a></td><td>$date</td><td>$this_from[$k]</td>\n<td><a href=\"$meurl?id=$v&user=$username\">$this_subj[$k]</a></td>\n<td><input type=\"checkbox\" name=\"$__v\"></td>\n</tr>\n";
      else
         print "<tr valign=\"top\">\n<td class=\"odd\"><a href=\"$meurl?id=$v&user=$username\">$K.</a></td><td class=\"odd\">$date</td><td class=\"odd\">$this_from[$k]</td>\n<td class=\"odd\"><a href=\"$meurl?id=$v&user=$username\">$this_subj[$k]</a></td>\n<td class=\"odd\"><input type=\"checkbox\" name=\"$__v\"></td>\n</tr>\n";
   }


   print "</table></p>\n";

   print "<p><input type=\"reset\" value=\"$CANCEL\">\n";
   print "<input type=\"button\" value=\"$SELECT_ALL\" onClick=\"mark_all(true)\"></p>\n";

   /*print "<p><a href=\"\" onClick='document.forms.aaa1.submit(); return false;'>$PURGE_SELECTED_MESSAGES</a>\n";
   print "<a href=\"\" onClick='document.forms.aaa1.action=\"massdeliver.php\"; document.forms.aaa1.submit(); return false;'>$DELIVER_SELECTED_MESSAGES</a>\n";
   print "<a href=\"\" onClick='document.forms.aaa1.action=\"masstrain.php\"; document.forms.aaa1.submit(); return false;'>$DELIVER_AND_TRAIN_SELECTED_MESSAGES</a>\n";
   print "</p>\n";*/

   print "<p><input type=\"submit\" value=\"$PURGE_SELECTED_MESSAGES\">\n";
   print "<input type=\"button\" value=\"$DELIVER_SELECTED_MESSAGES\" onClick='document.forms.aaa1.action=\"massdeliver.php\"; document.forms.aaa1.submit();'>\n";
   print "<input type=\"button\" value=\"$DELIVER_AND_TRAIN_SELECTED_MESSAGES\" onClick='document.forms.aaa1.action=\"masstrain.php\"; document.forms.aaa1.submit();'></p>\n";
   print "</form>\n";

   print "<form action=\"$meurl\" name=\"purgeallfromqueue\" method=\"post\">\n";
   print "<p><input type=\"hidden\" name=\"purgeallfromqueue\" value=\"1\">\n";
   print "<input type=\"hidden\" name=\"user\" value=\"$username\">\n";
   print "<input type=\"submit\" value=\"$PURGE_ALL_MESSAGES_FROM_QUARANTINE\"></p>\n";
   print "</form>\n";

   return $n_msgs;
}


function show_raw_message($dir, $id){

   $fp = fopen($dir . "/" . $id, "r");
   if($fp){
      while(($l = fgets($fp, 4096))){
         $l = preg_replace("/</", "&lt;", $l);
         $l = preg_replace("/>/", "&gt;", $l);
         print $l;
      }
      fclose($fp);
   }

}


function show_message($dir, $id){
   $header = "";
   $body_chunk = "";
   $is_header = 1;
   $state = "UNDEF";
   $b = array();
   $boundary = array();
   $text_plain = 0;
   $text_html = 0;
   $charset = "";
   $qp = $base64 = 0;

   $fp = fopen($dir . "/" . $id, "r");
   if($fp){
      while(($l = fgets($fp, 4096))){
         if($l[0] == "\r" && $l[1] == "\n" && $is_header == 1){
            print "<pre>$header</pre>\n\n";
            $is_header = 0;
         }

         if(preg_match("/^Content-Type:/i", $l)) $state = "CONTENT_TYPE";
         if(preg_match("/^Content-Transfer-Encoding:/i", $l)) $state = "CONTENT_TRANSFER_ENCODING";

         if($state == "CONTENT_TYPE"){
            $x = strstr($l, "boundary");
            if($x){
               $x = preg_replace("/\"/", "", $x);
               $x = preg_replace("/\'/", "", $x);
               $x = preg_replace("/ /", "", $x);

               $b = explode("boundary=", $x);
               array_push($boundary, rtrim($b[1]));
            }

            if(preg_match("/charset/i", $l)){
               $l = preg_replace("/\'/", "\"", $l);
               $x = explode("\"", $l);
               $charset = $x[1];
            }

            if(strstr($l, "text/plain")) $text_plain = 1;
            if(strstr($l, "text/html")) $text_html = 1;

         }

         if($state == "CONTENT_TRANSFER_ENCODING"){
            if(strstr($l, "quoted-printable")) $qp = 1;
            if(strstr($l, "base64")) $base64 = 1;
         }


         if($is_header == 1){
            if($l[0] != " " && $l[0] != "\t") $state = "UNDEF";
            if(preg_match("/^From:/", $l)) $state = "FROM";
            if(preg_match("/^To:/", $l)) $state = "TO";
            if(preg_match("/^Date:/", $l)) $state = "DATE";
            if(preg_match("/^Subject:/", $l)) $state = "SUBJECT";
            if(preg_match("/^Content-Type:/", $l)) $state = "CONTENT_TYPE";

            $l = preg_replace("/</", "&lt;", $l);
            $l = preg_replace("/>/", "&gt;", $l);

            if($state == "FROM" || $state == "TO" || $state == "SUBJECT" || $state == "DATE")
               $header .= decode_my_str($l) . "\r\n";

         }
         else {

            if(check_boundary($boundary, $l) == 1){
               flush_body_chunk($body_chunk, $charset, $qp, $text_plain, $text_html);

               $text_plain = $text_html = $qp = $base64 = 0;

               $charset = $body_chunk = "";

               continue;
            }

            else if($l[0] == "\r" && $l[1] == "\n"){
               $state = "BODY";
               $body_chunk .= $l;
            }

            else if($state == "BODY"){
               if($text_plain == 1 || $text_html == 1) $body_chunk .= $l;

            }

         }


      }
      fclose($fp);
   }

   if($body_chunk) flush_body_chunk($body_chunk, $charset, $qp, $text_plain, $text_html);

}


function check_boundary($boundary, $line){

   for($i=0; $i<count($boundary); $i++){
      if(strstr($line, $boundary[$i])) return 1;
   }

   return 0;
}


function flush_body_chunk($chunk, $charset, $qp, $text_plain, $text_html){
   if($qp == 1) $chunk = qp_decode($chunk);
   if(!preg_match("/utf-8/i", $charset)) $chunk = utf8_encode($chunk);

   if($text_plain == 1){
      $chunk = preg_replace("/</", "&lt;", $chunk);
      $chunk = preg_replace("/>/", "&gt;", $chunk);

      print "<pre>\n";
      print_nicely($chunk);
      print "</pre>\n";
   }

   if($text_html == 1){
      $chunk = preg_replace("/\<style\>([\w\W]+)\<\/style\>/i", "", $chunk);
      $chunk = preg_replace("/\<body ([\w\s\;\"\'\#\d\:\-\=]+)\>/i", "<body>", $chunk);

      print $chunk;
   }
}


function print_nicely($chunk){
   $k = 0;

   $x = explode(" ", $chunk);
   for($i=0; $i<count($x); $i++){
      print "$x[$i] ";
      $k += strlen($x[$i]);

      if(strstr($x[$i], "\n")) $k = 0;

      if($k > 70){ print "\n"; $k = 0; }
   }

}


function qp_decode($l){
   $res = "";
   $c = "";

   if($l == "") return "";

   /* remove soft breaks at the end of lines */
   if(preg_match("/\=\r\n/", $l)) $l = preg_replace("/\=\r\n/", "", $l);
   if(preg_match("/\=\n/", $l)) $l = preg_replace("/\=\n/", "", $l);

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


function show_users($queuedir){

   if($queuedir == "")
      return 0;

   if(!chdir($queuedir)) return 0;

   if($dh = opendir($queuedir)){
      while(($file = readdir($dh)) !== false){

         if($file != "." && $file != ".." && is_dir("$queuedir/$file")){

            if($dh2 = opendir("$queuedir/$file")){
               while(($file2 = readdir($dh2)) !== false){
                  if($file2 != "." && $file2 != ".."){
                     print "<a href=\"q.php?user=$file2\">$file2</a><br>\n";
                  }
               }
            }
            closedir($dh2);
         } 
      }
      closedir($dh);
   }

   return 1;
}

?>
