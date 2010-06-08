<?php


class ControllerStatCounter extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "stat/counter.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('stat/counter');

      $this->document->title = $this->data['text_counters'];


      if(Registry::get('admin_user') == 1) {

         if($this->request->server['REQUEST_METHOD'] == 'POST' && @$this->request->post['reset'] == 1) {
            $this->model_stat_counter->resetCounters();
            header("Location: index.php?route=stat/counter");
            exit;
         }

         $this->data['counters'] = $this->model_stat_counter->getCounters();

      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }


}

?>
