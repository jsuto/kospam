<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

include_once("header.php");

if($username == "") show_auth_popup();

if($backend != "mysql") nice_error($err_this_feature_is_not_available);

$timespan = 0;

$nham = 0;
$nspam = 0;

if(isset($_GET['timespan'])) $timespan = $_GET['timespan'];

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

<table border="0"><tr valign="top"><td>
<table border="1">

<?php
   print "<tr align=\"center\"><th>$DATE</th><th>HAM</th><th>SPAM</th></tr>\n";

   /* determine your uid */

   $conn = webui_connect() or nice_error($err_connect_db);

   $stmt = "SELECT uid FROM $user_table WHERE username='$username'";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($uid) = mysql_fetch_row($r);
   mysql_free_result($r);

   if($uid){
   if($timespan == 0)
      $stmt = "SELECT ts, SUM(nham), SUM(nspam) FROM $stat_table WHERE uid=$uid GROUP BY ts ORDER BY ts DESC LIMIT 24";
   else
      $stmt = "SELECT FROM_UNIXTIME(ts, '%Y.%m.%d.'), SUM(nham), SUM(nspam) FROM $stat_table WHERE uid=$uid GROUP BY FROM_UNIXTIME(ts, '%Y.%m.%d.') ORDER BY ts DESC LIMIT 30";

   $r = mysql_query($stmt) or nice_error($err_sql_error);
   while(list($ts, $ham, $spam) = mysql_fetch_row($r)){

      $nham += $ham;
      $nspam += $spam;

      if($timespan == 0){
         $ts += 70;
         //$ts = date("Y.m.d. H:i:s", $ts);
         $d1 = date("m.d.", $ts);
         $d2 = date("H:i", $ts);
         print "<tr align=\"center\"><td>$d1&nbsp;$d2</td><td>$ham</td><td>$spam</td></tr>\n";
      }
      else
         print "<tr align=\"center\"><td>$ts</td><td>$ham</td><td>$spam</td></tr>\n";

   }
   mysql_free_result($r);
   }

   webui_close($conn);

   print "<tr align=\"center\"><td><b>$TOTAL</b></td><td><b>$nham</b></td><td><b>$nspam</b></td></tr>\n";

?>

</table></td>

   <td><img src="graph.php?timespan=<?php print $timespan; ?>" /></td></tr></table>

</p>


      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<?php include_once("footer.php"); ?>
