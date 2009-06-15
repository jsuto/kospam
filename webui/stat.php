<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

if($username == "") show_auth_popup();

include_once("header.php");

if($userdb != "mysql") nice_error($err_this_feature_is_not_available);

$timespan = 0;
$uid = "";
$nham = 0;
$nspam = 0;

if(isset($_GET['timespan'])) $timespan = $_GET['timespan'];
if(isset($_GET['uid'])) $uid = $_GET['uid'];

?>


  <h3><?php print $STATS; ?></h3>

  <div id="body">

<p>

<?php
   if($timespan == 0)
         print "<p><strong>$CGI_DAILY_REPORT</strong> <a href=\"" . $_SERVER['PHP_SELF'] . "?timespan=1\">$CGI_MONTHLY_REPORT</a></p>\n";
      else
         print "<p><a href=\"" . $_SERVER['PHP_SELF'] ."\">$CGI_DAILY_REPORT</a> <strong>$CGI_MONTHLY_REPORT</strong><p/>\n";

?>

   <img src="graph.php?timespan=<?php print $timespan; ?>&uid=<?php print $uid; ?>" />

</p>


      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<?php include_once("footer.php"); ?>
