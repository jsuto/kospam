<?php


class ControllerQuarantineQuarantine extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/quarantine.tpl";
      $this->layout = "common/layout-quarantine";

      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->document->title = $this->data['text_search'];

      $this->data['searchtype'] = 'expert';

      if(isset($this->request->post['searchterm'])) {
      }

      $this->render();
   }


}

?>
