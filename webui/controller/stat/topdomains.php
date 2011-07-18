<?php


class ControllerStatTopdomains extends Controller {
   private $error = array();

   public function index(){

      $request = Registry::get('request');
      $db = Registry::get('db');
      $db_history = Registry::get('db_history');

      $this->load->model('user/user');

      $this->load->model('stat/chart');

      $this->load->helper('libchart/classes/libchart');

      $this->data['username'] = Registry::get('username');

      $what = @$this->request->get['what'];
      $timespan = @$this->request->get['timespan'];

      $emails = "";

      /* let the admin users see the whole statistics */

      $db->select_db($db->database);

      if(Registry::get('admin_user') == 0 && Registry::get('readonly_admin') == 0) {
         $uid = $this->model_user_user->getUidByName($this->data['username']);
         $emails = "AND rcpt IN ('" . preg_replace("/\n/", "','", $this->model_user_user->getEmailsByUid((int)$uid)) . "')";
      }
      else if(isset($this->request->get['uid']) && is_numeric($this->request->get['uid']) && $this->request->get['uid'] > 0){
         $emails = "AND rcpt IN ('" . preg_replace("/\n/", "','", $this->model_user_user->getEmailsByUid((int)$this->request->get['uid'])) . "')";
      }

      if(Registry::get('admin_user') == 1 || Registry::get('readonly_admin') == 1) {
         $db_history->select_db($db_history->database);

         $aa = new ModelStatChart();
         $aa->horizontalChartTopDomains($emails, $what, $timespan, "Top10 $what " . $this->data['text_sending_domains'], "");
      }

   }


}

?>
