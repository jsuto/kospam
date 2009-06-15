<?php

$lang = "en";
$WHERE = "";
$uid = "";


include_once("config.php");
include_once("libchart/classes/libchart.php");

session_start();
$username = get_authenticated_username();

if($username == "") show_auth_popup();

$admin_user = is_admin($username);

$title = $GRAPH['ham_and_spam_messages'];
$size_x = 800;
$size_y = 480;
$color_ham = "#1ac090";
$color_spam = "#d03080";

$timespan = 0;

$ydata = array();
$ydata2 = array();
$dates = array();

$chart = new LineChart($size_x, $size_y);
$line1 = new XYDataSet();
$line2 = new XYDataSet();


if(isset($_GET['timespan'])) $timespan = $_GET['timespan'];
if(isset($_GET['uid'])) $uid = $_GET['uid'];

$conn = webui_connect() or nice_error($err_connect_db);

if($admin_user == 0){
   $stmt = "SELECT uid FROM $user_table WHERE username='$username'";
   $r = mysql_query($stmt) or nice_error($err_sql_error);
   list($uid) = mysql_fetch_row($r);
   mysql_free_result($r);

   $WHERE = "WHERE uid=$uid";
}
else if($uid){
   if(!is_numeric($uid)) nice_error($err_NaN);
   $WHERE = "WHERE uid=$uid";
}

   if($timespan == 0)
      $stmt = "SELECT ts, SUM(nham), SUM(nspam) FROM $stat_table $WHERE GROUP BY ts ORDER BY ts DESC LIMIT 24";
   else
      $stmt = "SELECT ts, SUM(nham), SUM(nspam) FROM $stat_table $WHERE GROUP BY FROM_UNIXTIME(ts, '%Y.%m.%d.') ORDER BY ts DESC LIMIT 30";

   $i = 0;

   $r = mysql_query($stmt) or nice_error($err_sql_error);
   while(list($ts, $ham, $spam) = mysql_fetch_row($r)){
      $i++;

      if($timespan == 0){
         $ts += 70;
         $ts = date("H:i", $ts);
      }
      else
         $ts = date("m.d.", $ts);

      if($i % 3) $ts = "";

      array_push($ydata, $ham);
      array_push($ydata2, $spam);
      array_push($dates, $ts);
 
   }
   mysql_free_result($r);


webui_close($conn);

$ydata = array_reverse($ydata);
$ydata2 = array_reverse($ydata2);
$dates = array_reverse($dates);

for($i=0; $i<count($ydata); $i++){
   $ham = $ydata[$i];
   $spam = $ydata2[$i];
   $ts = $dates[$i];
   $line1->addPoint(new Point("$ts", $ham));
   $line2->addPoint(new Point("$ts", $spam));

}

$dataSet = new XYSeriesDataSet();
$dataSet->addSerie("HAM", $line1);
$dataSet->addSerie("SPAM", $line2);

$chart->setDataSet($dataSet);

$chart->setTitle($title);
$chart->getPlot()->setGraphCaptionRatio(0.80);
$chart->render();


?>

