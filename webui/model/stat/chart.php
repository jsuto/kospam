<?php

class ModelStatChart extends Model {

   public function lineChartHamSpam($emails, $timespan, $title, $output){
      $ydata = array();
      $ydata2 = array();
      $dates = array();

      $chart = new LineChart(SIZE_X, SIZE_Y);
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
         $i = 0;
         foreach ($query->rows as $q) {
            $i++;
            array_push($ydata, $q['num']);
            $q['ts'] = date("H:i", $q['ts']);

            if($i % 3){ $q['ts'] = ""; }

            array_push($dates, $q['ts']);
         }
      
         $query = $this->db_history->query("select ts-(ts%3600) as ts, count(*) as num from clapf where result='SPAM' $emails $grouping ORDER BY ts DESC limit $limit");
         foreach ($query->rows as $q) {
            array_push($ydata2, $q['num']);
         }
      }
      else {
         $query = $this->db_history->query("select ts-(ts%86400) as ts, count(*) as num from clapf where result='HAM' $emails $grouping ORDER BY ts DESC limit $limit");
         foreach ($query->rows as $q) {
            array_push($ydata, $q['num']);
            array_push($dates, date("m.d.", $q['ts']));
         }

         $query = $this->db_history->query("select ts-(ts%86400) as ts, count(*) as num from clapf where result='SPAM' $emails $grouping ORDER BY ts DESC limit $limit");
         foreach ($query->rows as $q) {
            array_push($ydata2, $q['num']);
            array_push($dates, date("m.d.", $q['ts']));
         }

      }


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

      $this->sendOutput($chart, $output);
   }


   public function pieChartHamSpam($timespan, $title, $output) {

      $range = $this->getRangeInSeconds($timespan);

      $chart = new PieChart(SIZE_X, SIZE_Y);
      $dataSet = new XYDataSet();

      $query = $this->db_history->query("SELECT COUNT(*) AS SPAM FROM clapf WHERE result='SPAM' AND ts > $range");
      if($query->num_rows > 0) { $dataSet->addPoint(new Point("SPAM (" . $query->row['SPAM'] . ")", $query->row['SPAM'])); }

      $query = $this->db_history->query("SELECT COUNT(*) AS HAM FROM clapf WHERE result='HAM' AND ts > $range");
      if($query->num_rows > 0) { $dataSet->addPoint(new Point("HAM (" . $query->row['HAM'] . ")", $query->row['HAM'])); }

      $chart->setDataSet($dataSet);
      $chart->setTitle($title);

      $this->sendOutput($chart, $output);
   }



   public function horizontalChartTopDomains($what, $timespan, $title, $output) {
      if($what != "HAM") { $what = "SPAM"; }

      $range = $this->getRangeInSeconds($timespan);

      $chart = new HorizontalBarChart(SIZE_X, SIZE_Y);
      $dataSet = new XYDataSet();

      $query = $this->db_history->query("SELECT from_domain, COUNT(from_domain) AS sum FROM " . TABLE_SUMMARY . " WHERE ts > $range AND result='" . $this->db_history->escape($what) . "' GROUP BY from_domain ORDER BY sum DESC LIMIT 10");

      foreach($query->rows as $q) {
         $dataSet->addPoint(new Point($q['from_domain'], $q['sum']));
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

      $chart->render($output);
   }

}

?>
