<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

if($username == "") show_auth_popup();

include_once("header.php");

if(isset($_COOKIE['pagelen'])){
   if($_COOKIE['pagelen'] >= 10 && $_COOKIE['pagelen'] <= 50) $page_len = $_COOKIE['pagelen'];
}

$meurl = $_SERVER['PHP_SELF'];

$page = 0;
$user = "";
$from = "";
$subj = "";
$id = "";
$remove = "";
$deliver = "";
$train = "";

$topurge = 0;
$purgeallfromqueue = 0;

if(isset($_GET['page'])) $page = $_GET['page'];
if(isset($_GET['user'])) $user = $_GET['user'];
if(isset($_GET['from'])) $from = $_GET['from'];
if(isset($_GET['subj'])) $subj = $_GET['subj'];
if(isset($_GET['id'])) $id = $_GET['id'];
if(isset($_GET['remove'])) $remove = $_GET['remove'];
if(isset($_GET['deliver'])) $deliver = $_GET['deliver'];
if(isset($_GET['train'])) $train = $_GET['train'];

if(isset($_POST['topurge'])) $topurge = $_POST['topurge'];
if(isset($_POST['purgeallfromqueue'])) $purgeallfromqueue = $_POST['purgeallfromqueue'];
if(isset($_POST['user'])) $user = $_POST['user'];

/* fix the username if you are an admin user */
if($admin_user == 1 && $user) $username = $user;

$my_q_dir = $queue_directory . "/" . substr($username, 0, 1) . "/$username";

if($id && !preg_match('/^(s\.[0-9a-f]+)$/', $id)) nice_error($err_invalid_message_id);
if($remove && !preg_match('/^(s\.[0-9a-f]+)$/', $remove)) nice_error($err_invalid_message_id);
if($deliver && !preg_match('/^(s\.[0-9a-f]+)$/', $deliver)) nice_error($err_invalid_message_id);

if(!is_numeric($page) || $page < 0) $page = 0;

$conn = webui_connect() or nice_error($err_connect_db);

?>

<script type="text/javascript">

function mark_all(x){
   var i;
   var len = document.aaa1.elements.length;

   for(i=0; i<len; i++)
      document.aaa1.elements[i].checked = x;
}

</script>


  <h3><?php print $QUARANTINE; ?></h3>

  <div id="body">

<p>

<?php

if($_SERVER['REQUEST_METHOD'] == "GET"){

   /* show userlist if you are an admin user */

   if($admin_user == 1 && $user == ""){
      print "<h4>$USERLIST</h4>\n";

      show_users($queue_directory);
   }

   /* show selected message ... */

   else if($id){
      print "<p><a href=\"$meurl?remove=$id&user=$username\">$REMOVE</a> <a href=\"$meurl?deliver=$id&user=$username\">$DELIVER</a> <a href=\"$meurl?train=$id&user=$username\">$TRAIN_AND_DELIVER</a></p>\n";

      print "<pre>\n";
      show_message($my_q_dir, $id);
      print "</pre>\n";
   }

   /* release message from quarantine */

   else if($train || $deliver){
      if($username == "aaa") nice_error("I don't deliver for the demo user....");

      if($train) $deliver = $train;

      $m = get_message_for_delivery($my_q_dir . "/$deliver", $fromaddr);

      if($train){
         $fromaddr = get_users_email_address($username);
         $id = preg_replace("/s./", "", $train);

         $m2 = "From: $fromaddr\r\nTo: $ham_train_address\r\nSubject: training a spam as ham\r\n\r\n\r\n";
         $m2 .= "$clapf_header_field$id\r\n" . $m;

         $x = send_smtp_email($smtphost, $clapfport, $yourdomain, $fromaddr, $ham_train_address, $m2);
         if(!$x) nice_error("$err_message_failed_to_train");
      }


      /* get user's email address */

      $to = get_users_email_address($username);

      $x = send_smtp_email($smtphost, $smtpport, $yourdomain, $fromaddr, $to, $m);

      if($x == 1){
         @unlink($my_q_dir . "/" . $deliver);
         nice_screen("$err_message_delivered $to");
      }
      else nice_error("$err_message_failed_to_deliver $to");
   }


   /* remove selected message ... */

   else if($remove){
      if(unlink($my_q_dir . "/" . $remove)) print "<p>$SUCCESSFULLY_REMOVED ($remove).</p>\n<a href=\"$meurl?user=$username\">$BACK.</a>\n";
      else print "<p>$FAILED_TO_REMOVE ($remove).</p>\n<a href=\"$meurl?user=$username\">BACK.</a>\n";
   }


   /* or scan directory */

   else {


      print "<form action=\"$meurl\" name=\"aaa0\" method=\"get\">\n";
      print "<input type=\"hidden\" name=\"user\" value=\"$username\">\n";
      print "<table border=\"0\">\n";
      print "<tr><td>$FROM:</td><td><input type=\"text\" name=\"from\" value=\"$from\"></td></tr>\n";
      print "<tr><td>$SUBJECT:</td><td><input type=\"text\" name=\"subj\" value=\"$subj\"></td></tr>\n";
      print "<tr colspan=\"2\"><td><input type=\"submit\" value=\"OK\"></td></tr>\n";
      print "</table>\n";
      print "</form>\n";


      $nspam = check_directory($my_q_dir, $username, $page, $from, $subj);

      print "<p>\n";
      $prev_page = $page - 1;
      $next_page = $page + 1;
      $total_pages = floor($nspam/$page_len);

      if($page > 0)
         print "<a href=\"$meurl?page=0&user=$username&from=$from&subj=$subj\">$FIRST</a> <a href=\"$meurl?page=$prev_page&user=$username&from=$from&subj=$subj\">$PREVIOUS</a>\n";

      if($nspam >= $page_len*($page+1) && $nspam > $page_len)
         print " <a href=\"$meurl?page=$next_page&user=$username&from=$from&subj=$subj\">$NEXT</a>\n";

      if($page < $nspam/$page_len && $nspam > $page_len)
         print " <a href=\"$meurl?page=$total_pages&user=$username&from=$from&subj=$subj\">$LAST</a>\n";

      print "</p>\n";

   }
}

if($_SERVER['REQUEST_METHOD'] == "POST"){
   if($topurge == 1){
      $n = 0;

      while(list($k, $v) = each($_POST)){

         if(preg_match('/^([0-9a-f]+)$/', $k)){
            //print "aa: $k * $v<br>\n";

            if(unlink($my_q_dir . "/s." . $k)) $n++;
         }
      }

      print "$PURGED: $n. <a href=\"q.php?user=$username\">$BACK</a>";
   }


   if($purgeallfromqueue == 1){
      $n = 0;

      $files = scandir($my_q_dir, 1);

      while(list($k, $v) = each($files)){
         if(strncmp($v, "s.", 2) == 0){

            $f = $my_q_dir . "/" . $v;
            if(preg_match('/^s\.([0-9a-f]+)$/', $v)){
               if(unlink($f)) $n++;
            }
         }
      }
      print "$PURGED: $n. <a href=\"q.php?user=$username\">$BACK</a>";
   }

}

webui_close($conn);

?>

</p>


      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- content -->




<?php include_once("footer.php"); ?>
