<?php


class ControllerHistoryHistory extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "history/history.tpl";
      $this->layout = "common/layout-history";


      $request = Registry::get('request');
      //$db = Registry::get('db');
      $language = Registry::get('language');

      $this->document->title = $language->get('text_history');


      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {


      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }

}

?>
