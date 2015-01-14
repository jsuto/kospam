<?php


class ControllerStatChart extends Controller {
   private $error = array();

   public function index(){

      $request = Registry::get('request');
      $db = Registry::get('db');
      $db_history = Registry::get('db_history');

      $this->load->model('stat/chart');

      $this->load->helper('libchart/classes/libchart');

      $this->data['username'] = Registry::get('username');

      $timespan = @$this->request->get['timespan'];

      $aa = new ModelStatChart();
      $aa->pieChartHamSpam($timespan, $this->data['text_ham_and_spam_messages'], "");

   }


}

?>
