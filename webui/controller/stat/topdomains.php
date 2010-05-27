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

      $this->load->model('stat/chart');

      $this->load->helper('libchart/classes/libchart');

      $this->data['username'] = Registry::get('username');

      $what = @$this->request->get['what'];
      $timespan = @$this->request->get['timespan'];

      if(Registry::get('admin_user') == 1) {
         $aa = new ModelStatChart();
         $aa->horizontalChartTopDomains($what, $timespan, "Top $what sending domains", "");
      }

   }


}

?>
