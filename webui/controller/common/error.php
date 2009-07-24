<?php


class ControllerCommonError extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "common/error.tpl";
      $this->layout = "common/layout";

      $this->document->title = $this->data['title_error'];

      $this->data['errortitle'] = $this->data['title_error'];

      $this->data['errorstring'] = "this is the errorstring";



      $this->render();

   }


}


?>
