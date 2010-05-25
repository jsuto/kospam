<?php


class ControllerStatTopdomains extends Controller {
   private $error = array();

   public function index(){

      $request = Registry::get('request');
      $db_history = Registry::get('db_history');

      if(DB_DRIVER == "ldap")
         $this->load->model('user/ldap/user');
      else
         $this->load->model('user/sql/user');

      $this->load->helper('libchart/classes/libchart');

      $this->data['username'] = Registry::get('username');


      $range = time() - 604800;
      $what = "SPAM";

      $timespan = @$this->request->get['timespan'];

      if(@$this->request->get['what'] == "HAM") { $what = $this->request->get['what']; }

      if($timespan == "daily") { $range = time() - 86400; }
      if($timespan == "monthly") { $range = time() - 2592000; }


      $chart = new HorizontalBarChart(SIZE_X, SIZE_Y);
      $dataSet = new XYDataSet();

      $query = $this->db_history->query("SELECT from_domain, COUNT(from_domain) AS sum FROM " . TABLE_SUMMARY . " WHERE ts > $range AND result='" . $this->db_history->escape($what) . "' GROUP BY from_domain ORDER BY sum DESC LIMIT 10");

      foreach($query->rows as $q) {
         $dataSet->addPoint(new Point($q['from_domain'], $q['sum']));
      }



      $chart->setDataSet($dataSet);
      $chart->setTitle("Top $what sending domains");

      header("Content-type: image/png");
      header("Expires: now");

      $chart->render();

   }


}

?>
