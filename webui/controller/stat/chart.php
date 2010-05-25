<?php


class ControllerStatChart extends Controller {
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

      $timespan = @$this->request->get['timespan'];

      if($timespan == "daily") { $range = time() - 86400; }
      if($timespan == "monthly") { $range = time() - 2592000; }


      $title = $this->data['text_ham_and_spam_messages'];

      $chart = new PieChart(SIZE_X, SIZE_Y);
      $dataSet = new XYDataSet();

      $query = $this->db_history->query("SELECT COUNT(*) AS SPAM FROM clapf WHERE result='SPAM' AND ts > $range");
      if($query->num_rows > 0) { $dataSet->addPoint(new Point("SPAM (" . $query->row['SPAM'] . ")", $query->row['SPAM'])); }

      $query = $this->db_history->query("SELECT COUNT(*) AS HAM FROM clapf WHERE result='HAM' AND ts > $range");
      if($query->num_rows > 0) { $dataSet->addPoint(new Point("HAM (" . $query->row['HAM'] . ")", $query->row['HAM'])); }

      $chart->setDataSet($dataSet);
      $chart->setTitle($title);

      header("Content-type: image/png");
      header("Expires: now");

      $chart->render();

   }


}

?>
