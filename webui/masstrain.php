<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

if($username == "") show_auth_popup();

include_once("header.php");

$meurl = $_SERVER['PHP_SELF'];

$user = "";
$deliver = "";

if(isset($_GET['user'])) $user = $_GET['user'];
if(isset($_GET['deliver'])) $deliver = $_GET['deliver'];

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

  <h3><?php print $QUARANTINE; ?></h3>

  <div id="body">

<p>

<?php


/* release message(s) from quarantine */

$fromaddr = get_users_email_address($username);
$to = get_users_email_address($username);
$n = 0;


while(list($k, $v) = each($_POST)){
   if(preg_match("/^[a-f0-9]{28,36}$/", $k) && preg_match("/^on$/i", $v)){

      $deliver = "s." . $k;
      $m = get_message_for_delivery($my_q_dir . "/$deliver", $fromaddr);


      $m2 = "From: $fromaddr\r\nTo: $ham_train_address\r\nSubject: training a spam as ham\r\n\r\n\r\n";
      $m2 .= "$clapf_header_field$k\r\n" . $m;

      $x = send_smtp_email($smtphost, $clapfport, $yourdomain, $fromaddr, $ham_train_address, $m2);

      $x = send_smtp_email($smtphost, $smtpport, $yourdomain, $fromaddr, $to, $m);

      if($x == 1){
         @unlink($my_q_dir . "/" . $deliver);
         $n++;
      }
   }
}

print "$DELIVERED: $n $MESSAGES. <a href=\"q.php?user=$username\">$BACK</a>";


webui_close($conn);

?>

</p>


      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<?php include_once("footer.php"); ?>
