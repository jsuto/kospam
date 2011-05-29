<?php


class ControllerHistoryHistory extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "history/history.tpl";
      $this->layout = "common/layout-history";


      $request = Registry::get('request');
      $language = Registry::get('language');

      $this->document->title = $language->get('text_history');


      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {
         $this->data['hamspam'] = @$this->request->cookie['hamspam'];
         $this->data['sender_domain'] = @$this->request->cookie['sender_domain'];
         $this->data['rcpt_domain'] = @$this->request->cookie['rcpt_domain'];
         $this->data['date1'] = @$this->request->cookie['date1'];
         $this->data['date2'] = @$this->request->cookie['date2'];

         if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
            $this->data['page'] = $this->request->get['page'];
         }
         else {
            $this->data['page'] = 0;
         }

      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }

}

?>
