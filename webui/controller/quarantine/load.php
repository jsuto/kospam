<?php


class ControllerQuarantineLoad extends Controller {
   private $error = array();


   public function index(){

      $this->id = "content";
      $this->template = "quarantine/load.tpl";
      $this->layout = "common/layout-empty";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('user/user');
      $this->load->model('quarantine/database');

      $uid = $this->model_user_user->getUidByName(Registry::get('username'));

      $this->data['terms'] = $this->model_quarantine_database->get_search_terms($uid);

      $this->render();
   }

}


?>
