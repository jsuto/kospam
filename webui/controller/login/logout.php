<?php


class ControllerLoginLogout extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "login/logout.tpl";
      $this->layout = "common/layout-empty";

      $request = Registry::get('request');
      $session = Registry::get('session');

      $db = Registry::get('db');

      $this->load->model('user/auth');
      $this->load->model('stat/online');

      $this->data['title'] = $this->data['text_logout'];
      $this->data['title_prefix'] = TITLE_PREFIX;

      $this->model_stat_online->offline($session->get('email'));

      logout();

      $this->render();
   }


}

?>
