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

      $this->load->model('stat/chart');

      $this->load->helper('libchart/classes/libchart');

      $this->data['username'] = Registry::get('username');



      $aa = new ModelStatChart();

      $timespan = @$this->request->get['timespan'];

      $aa->pieChartHamSpam($timespan, $this->data['text_ham_and_spam_messages'], "");

   }


}

?>
