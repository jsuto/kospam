<?


function errout($msg){
   print $msg;

   exit(1);
}


function nice_error($msg){
   global $ERROR;

   include_once("header.inc");

   print "<h3>$ERROR</h3>\n";

   print "<div id=\"body\">\n\n\n";

   print "<p>$msg</p>\n\n";

   print "</div> <!-- body -->\n\n\n";

   include_once("footer.inc");

   exit(1);
}


function nice_screen($msg){
   include_once("header.inc");

   print "<h3>OK</h3>\n";

   print "<div id=\"body\">\n\n\n";

   print "<p>$msg</p>\n\n";

   print "</div> <!-- body -->\n\n\n";

   include_once("footer.inc");

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
   fputs($r, $msg . "\n.\r\n");

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



function scan_message($dir, $f){
   global $NO_SENDER, $NO_SUBJECT, $max_cgi_from_subj_len;

   $from = $NO_SENDER;
   $subj = $NO_SUBJECT;

   $i = 0;


   $fp = fopen($dir . "/" . $f, "r");
   if($fp){
      while(($l = fgets($fp, 4096))){
         if(strncmp($l, "Subject:", 8) == 0 && strlen($l) > 10){
            $subj = substr($l, 9, $max_cgi_from_subj_len);
            if(strlen($l) > 9+$max_cgi_from_subj_len) $subj .= "...";
            $i++;
         }
         if(strncmp($l, "From:", 5) == 0 && strlen($l) > 10){
            $from = substr($l, 6, $max_cgi_from_subj_len);
            if(strlen($l) > 6+$max_cgi_from_subj_len) $from .= "...";
            $i++;
         }

         if($i >= 2)
            break;
      }
      fclose($fp);
   }

   $from = preg_replace("/</", "[", $from);
   $from = preg_replace("/>/", "]", $from);

   return array ($from, $subj);
}


function get_message_for_delivery($f, $new_from){
   $msg = "";

   $fp = fopen($f, "r");
   if($fp){
      while(($l = fgets($fp, 4096))){
         if(strncmp($l, "From:", 5) == 0)
            $msg .= "From: $new_from\r\n";         
         else
            $msg .= $l;
      }
      fclose($fp);
   }

   return $msg;
}


function check_directory($dir, $username, $page, $from, $subj){
   global $page_len, $max_cgi_from_subj_len, $NUMBER_OF_SPAM_MESSAGES_IN_QUARANTINE, $DATE, $PURGE_SELECTED_MESSAGES, $CANCEL, $SELECT_ALL;

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

   print "<table border=\"0\">\n";
   print "<tr align=\"middle\"><th>&nbsp;</th><th>$DATE</th><th>FROM</th><th>SUBJECT</th><th>&nbsp;</th></tr>\n";
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


   print "</table>\n";

   print "<p><input type=\"submit\" value=\"$PURGE_SELECTED_MESSAGES\"> <input type=\"reset\" value=\"$CANCEL\">\n";
   print "<input type=\"button\" value=\"$SELECT_ALL\" onClick=\"mark_all(true)\"></form></p>\n";

   return $n_msgs;
}


function show_message($dir, $id){

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
