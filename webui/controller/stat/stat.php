<?php


class ControllerStatStat extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "stat/stat.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('user/user');

      $this->document->title = $this->data['text_statistics'];

      $this->data['timespan'] = (int)@$this->request->get['timespan'];
      $this->data['uid'] = @$this->request->get['uid'];

      $this->render();
   }


}

?>
