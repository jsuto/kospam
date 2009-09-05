<?php


class ControllerStatGraph extends Controller {
   private $error = array();

   public function index(){

      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('user/user');

      $this->load->helper('libchart/classes/libchart');

      $this->data['username'] = Registry::get('username');

      $timespan = (int)@$this->request->get['timespan'];


      $title = $this->data['text_ham_and_spam_messages'];
      $size_x = 800;
      $size_y = 480;
      $color_ham = "#1ac090";
      $color_spam = "#d03080";

      $ydata = array();
      $ydata2 = array();
      $dates = array();

      $chart = new LineChart($size_x, $size_y);
      $line1 = new XYDataSet();
      $line2 = new XYDataSet();

      $WHERE = "";


      /* let the admin users see the whole statistics */

      if(Registry::get('admin_user') == 0) {
         $uid = $this->model_user_user->getUidByName($this->data['username']);
         $WHERE = "WHERE uid=" . (int)$uid;
      }
      else if(isset($this->request->get['uid']) && is_numeric($this->request->get['uid']) && $this->request->get['uid'] > 0){
         $WHERE = "WHERE uid=" . (int)$this->request->get['uid'];
      }

      if($timespan == 0){
         $query = $this->db->query("SELECT ts, SUM(nham) AS ham, SUM(nspam) AS spam FROM " . TABLE_STAT . " $WHERE GROUP BY ts ORDER BY ts DESC LIMIT 24");
      }
      else {
         $query = $this->db->query("SELECT ts, SUM(nham) AS ham, SUM(nspam) AS spam FROM " . TABLE_STAT . " $WHERE GROUP BY FROM_UNIXTIME(ts, '%Y.%m.%d.') ORDER BY ts DESC LIMIT 30");
      }

      $i = 0;

      foreach ($query->rows as $q) {

         $i++;

         if($timespan == 0){
            $q['ts'] += 70;
            $q['ts'] = date("H:i", $q['ts']);
         }
         else {
            $q['ts'] = date("m.d.", $q['ts']);
         }

         if($i % 3){ $q['ts'] = ""; }

         array_push($ydata, $q['ham']);
         array_push($ydata2, $q['spam']);
         array_push($dates, $q['ts']);
 
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
      $chart->render();

   }


}

?>
