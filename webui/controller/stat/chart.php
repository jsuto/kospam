<?php


class ControllerStatChart extends Controller {
   private $error = array();

   public function index(){

      $request = Registry::get('request');
      $db_history = Registry::get('db_history');

      $this->load->model('user/user');

      $this->load->model('stat/chart');

      $this->load->helper('libchart/classes/libchart');

      $this->data['username'] = Registry::get('username');

      $timespan = @$this->request->get['timespan'];

      $emails = "";

      /* let the admin users see the whole statistics */

      if(Registry::get('admin_user') == 0) {
         $uid = $this->model_user_user->getUidByName($this->data['username']);
         $emails = "AND rcpt IN ('" . preg_replace("/\n/", "','", $this->model_user_user->getEmailsByUid((int)$uid)) . "')";
      }
      else if(isset($this->request->get['uid']) && is_numeric($this->request->get['uid']) && $this->request->get['uid'] > 0){
         $emails = "AND rcpt IN ('" . preg_replace("/\n/", "','", $this->model_user_user->getEmailsByUid((int)$this->request->get['uid'])) . "')";
      }

      $aa = new ModelStatChart();
      $aa->pieChartHamSpam($emails, $timespan, $this->data['text_ham_and_spam_messages'], "");

   }


}

?>
