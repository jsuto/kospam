<?php

class ModelStatChart extends Model {

   public function lineChartHamSpam($emails, $timespan, $title, $size_x, $size_y, $output){
      $ydata = array();
      $ydata2 = array();
      $dates = array();

      $chart = new LineChart($size_x, $size_y);

      $chart->getPlot()->getPalette()->setLineColor(array(
         new Color(26, 192, 144),
         new Color(208, 48, 128),
      ));

      $line1 = new XYDataSet();
      $line2 = new XYDataSet();

      $limit = $this->getDataPoints($timespan);

      if(HISTORY_DRIVER == "sqlite"){
         if($timespan == "daily"){ $grouping = "GROUP BY strftime('%Y.%m.%d %H', datetime(ts, 'unixepoch'))"; }
         else { $grouping = "GROUP BY strftime('%Y.%m.%d', datetime(ts, 'unixepoch'))"; }
      }
      else {
         if($timespan == "daily"){ $grouping = "GROUP BY FROM_UNIXTIME(ts, '%Y.%m.%d. %H')"; }
         else { $grouping = "GROUP BY FROM_UNIXTIME(ts, '%Y.%m.%d.')"; }
      }


      if($timespan == "daily"){
         $query = $this->db_history->query("select ts-(ts%3600) as ts, count(*) as num from clapf where result='HAM' $emails $grouping ORDER BY ts DESC limit $limit");
         $query2 = $this->db_history->query("select ts-(ts%3600) as ts, count(*) as num from clapf where result='SPAM' $emails $grouping ORDER BY ts DESC limit $limit");
         $date_format = "H:i";
      } else {
         $query = $this->db_history->query("select ts-(ts%86400) as ts, count(*) as num from clapf where result='HAM' $emails $grouping ORDER BY ts DESC limit $limit");
         $query2 = $this->db_history->query("select ts-(ts%86400) as ts, count(*) as num from clapf where result='SPAM' $emails $grouping ORDER BY ts DESC limit $limit");
         $date_format = "m.d.";
      }

      foreach ($query->rows as $q) {
         array_push($ydata, $q['num']);
         array_push($dates, date($date_format, $q['ts']));
      }

      foreach ($query2->rows as $q) {
         array_push($ydata2, $q['num']);
         array_push($dates, date($date_format, $q['ts']));
      }

      if($query->num_rows >= 15) {
         $i = 0;
         while(list($k, $v) = each($dates)) {
            $i++;
            if($i % 3) { $dates[$k] = ""; }
         }
         reset($dates);
      }



      $ydata = array_reverse($ydata);
      $ydata2 = array_reverse($ydata2);
      $dates = array_reverse($dates);

      for($i=0; $i<count($ydata); $i++){
         $ham = $ydata[$i];
         if(isset($ydata2[$i])) { $spam = $ydata2[$i]; } else { $spam = 0; }
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

      $this->sendOutput($chart, $output);
   }


   public function pieChartHamSpam($emails = '', $timespan, $title, $output) {
      $ham = $spam = 0;

      $range = $this->getRangeInSeconds($timespan);

      $chart = new PieChart(SIZE_X, SIZE_Y);

      $query = $this->db_history->query("SELECT COUNT(*) AS SPAM FROM clapf WHERE result='SPAM' $emails AND ts > $range");
      if($query->num_rows > 0) { $spam = $query->row['SPAM']; }

      $query = $this->db_history->query("SELECT COUNT(*) AS HAM FROM clapf WHERE result='HAM' $emails AND ts > $range");
      if($query->num_rows > 0) { $ham = $query->row['HAM']; }

      if($ham > $spam) {
         $chart->getPlot()->getPalette()->setPieColor(array(new Color(26, 192, 144), new Color(208, 48, 128) ));
      } else {
         $chart->getPlot()->getPalette()->setPieColor(array(new Color(208, 48, 128), new Color(26, 192, 144) ));
      }


      $dataSet = new XYDataSet();

      $dataSet->addPoint(new Point("HAM ($ham)", $ham));
      $dataSet->addPoint(new Point("SPAM ($spam)", $spam));

      $chart->setDataSet($dataSet);
      $chart->setTitle($title);

      $this->sendOutput($chart, $output);
   }



   public function horizontalChartTopDomains($emails = '', $what, $timespan, $title, $output) {
      if($what != "HAM") { $what = "SPAM"; }

      $range = $this->getRangeInSeconds($timespan);

      $chart = new HorizontalBarChart(SIZE_X, SIZE_Y);
      $dataSet = new XYDataSet();

      $query = $this->db_history->query("SELECT ts, fromdomain, COUNT(fromdomain) AS sum FROM clapf WHERE ts > $range $emails AND result='" . $this->db_history->escape($what) . "' GROUP BY fromdomain ORDER BY sum DESC LIMIT 10");

      foreach($query->rows as $q) {
         $dataSet->addPoint(new Point($q['fromdomain'], $q['sum']));
      }

      $chart->setDataSet($dataSet);
      $chart->setTitle($title);

      $this->sendOutput($chart, $output);
   }


   private function getRangeInSeconds($timespan) {
      $range = 0;

      if($timespan == "daily") { return time() - 86400; }
      if($timespan == "weekly") { return time() - 604800; }

      return time() - 2592000;
   }


   private function getDataPoints($timespan) {

      if($timespan == "daily") { return 24; }
      if($timespan == "weekly") { return 7; }

      return 30;
   }


   private function sendOutput($chart, $output = '') {
      if($output == "") {
         header("Content-type: image/png");
         header("Expires: now");
      }

      if($output) {
         $chart->render($output);
      }
      else {
         $chart->render();
      }
   }

}

?>
