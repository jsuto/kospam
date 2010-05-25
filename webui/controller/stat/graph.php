<?php


class ControllerStatGraph extends Controller {
   private $error = array();

   public function index(){

      $request = Registry::get('request');
      $db = Registry::get('db');
      $db_history = Registry::get('db_history');

      if(DB_DRIVER == "ldap")
         $this->load->model('user/ldap/user');
      else
         $this->load->model('user/sql/user');

      $this->load->helper('libchart/classes/libchart');

      $this->data['username'] = Registry::get('username');

      $timespan = @$this->request->get['timespan'];


      $title = $this->data['text_ham_and_spam_messages'];

      $ydata = array();
      $ydata2 = array();
      $dates = array();

      $chart = new LineChart(SIZE_X, SIZE_Y);
      $line1 = new XYDataSet();
      $line2 = new XYDataSet();

      $emails = "";

      /* let the admin users see the whole statistics */

      if(Registry::get('admin_user') == 0) {
         $uid = $this->model_user_user->getUidByName($this->data['username']);
         $emails = "AND rcpt IN ('" . preg_replace("/\n/", "','", $this->model_user_user->getEmailsByUid((int)$uid)) . "')";
      }
      else if(isset($this->request->get['uid']) && is_numeric($this->request->get['uid']) && $this->request->get['uid'] > 0){
         $emails = "AND rcpt IN ('" . preg_replace("/\n/", "','", $this->model_user_user->getEmailsByUid((int)$this->request->get['uid'])) . "')";
      }


      if(HISTORY_DRIVER == "sqlite"){
         if($timespan == "daily"){ $grouping = "GROUP BY strftime('%Y.%m.%d %H', datetime(ts, 'unixepoch'))"; }
         else { $grouping = "GROUP BY strftime('%Y.%m.%d', datetime(ts, 'unixepoch'))"; }
      }
      else {
         if($timespan == "daily"){ $grouping = "GROUP BY FROM_UNIXTIME(ts, '%Y.%m.%d. %H')"; }
         else { $grouping = "GROUP BY FROM_UNIXTIME(ts, '%Y.%m.%d.')"; }
      }

      if($timespan == "daily"){
         $query = $this->db_history->query("select ts-(ts%3600) as ts, count(*) as num from clapf where result='HAM' $emails $grouping ORDER BY ts DESC limit 24");
         $i = 0;
         foreach ($query->rows as $q) {
            $i++;
            array_push($ydata, $q['num']);
            $q['ts'] = date("H:i", $q['ts']);

            if($i % 3){ $q['ts'] = ""; }

            array_push($dates, $q['ts']);
         }

         $query = $this->db_history->query("select ts-(ts%3600) as ts, count(*) as num from clapf where result='SPAM' $emails $grouping ORDER BY ts DESC limit 24");
         foreach ($query->rows as $q) {
            array_push($ydata2, $q['num']);
            //array_push($dates, date("H:i", $q['ts']));
         }

      }
      else {
         $query = $this->db_history->query("select ts-(ts%86400) as ts, count(*) as num from clapf where result='HAM' $emails $grouping ORDER BY ts DESC limit 30");
         foreach ($query->rows as $q) {
            array_push($ydata, $q['num']);
            array_push($dates, date("m.d.", $q['ts']));
         }

         $query = $this->db_history->query("select ts-(ts%86400) as ts, count(*) as num from clapf where result='SPAM' $emails $grouping ORDER BY ts DESC limit 30");
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

      header("Content-type: image/png");
      header("Expires: now");

      $chart->render();

   }


}

?>
