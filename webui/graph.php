<?php

$lang = "en";

include_once(".htdb.php");
include_once("auth.php");
include_once("mysql.php");
include_once("lang/$lang/messages.php");

include_once('jpgraph/src/jpgraph.php');
include_once('jpgraph/src/jpgraph_line.php');

session_start();
$username = get_authenticated_username();


if($username == "") show_auth_popup();

$title = $GRAPH['ham_and_spam_messages'];
$size_x = 600;
$size_y = 480;
$color_ham = "#d03080";
$color_spam = "#1ac090";

$timespan = 0;

$ydata = array();
$ydata2 = array();
$dates = array();

if(isset($_GET['timespan'])) $timespan = $_GET['timespan'];

$conn = webui_connect() or nice_error($err_connect_db);

$stmt = "SELECT uid FROM $user_table WHERE username='$username'";
$r = mysql_query($stmt) or nice_error($err_sql_error);
list($uid) = mysql_fetch_row($r);
mysql_free_result($r);

if($uid){
   if($timespan == 0)
      $stmt = "SELECT ts, SUM(nham), SUM(nspam) FROM $stat_table WHERE uid=$uid GROUP BY ts ORDER BY ts DESC LIMIT 24";
   else
      $stmt = "SELECT ts, SUM(nham), SUM(nspam) FROM $stat_table WHERE uid=$uid GROUP BY FROM_UNIXTIME(ts, '%Y.%m.%d.') ORDER BY ts DESC LIMIT 30";

   $r = mysql_query($stmt) or nice_error($err_sql_error);
   while(list($ts, $ham, $spam) = mysql_fetch_row($r)){

      if($timespan == 0){
         $ts += 70;
         $ts = date("H:i", $ts);
      }
      else
         $ts = date("m.d.", $ts);

      array_push($dates, $ts);

      array_push($ydata, $ham);
      array_push($ydata2, $spam);

   }
   mysql_free_result($r);
}

webui_close($conn);

$ydata = array_reverse($ydata);
$ydata2 = array_reverse($ydata2);
$dates = array_reverse($dates);

$graph = new Graph($size_x, $size_y, "auto");

$graph->img->SetImgFormat("png");
$graph->SetShadow();
$graph->SetScale("textlin");

$graph->xaxis->SetTickLabels($dates);
$graph->xaxis->SetTextLabelInterval(4);
$graph->xaxis->SetLabelAngle(0);

$lineplot = new LinePlot($ydata);
$lineplot->SetColor($color_ham);
$lineplot->SetWeight(5);

$graph->img->SetMargin(100, 40, 40, 40);
$graph->title->Set($title);


$lineplot = new LinePlot($ydata);
$lineplot->SetColor($color_ham);
$lineplot->SetWeight(5);


$lineplot2 = new LinePlot($ydata2);

$lineplot2->SetFillColor($color_spam);
$lineplot2->mark->SetWidth(4);

$lineplot->SetLegend("HAM");
$lineplot2->SetLegend("SPAM");

$graph->Add($lineplot2);
$graph->Add($lineplot);



$graph->legend->Pos(0.07, 0.95, "center", "bottom");

$graph->Stroke();

?>

