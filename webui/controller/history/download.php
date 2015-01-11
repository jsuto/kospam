<?php


class ControllerHistoryDownload extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "message/headers.tpl";
      $this->layout = "common/layout-empty";

      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('search/search');

      if(Registry::get('admin_user') == 0 && Registry::get('auditor_user') == 0) {
         die("go away");
      }

      $this->document->title = $this->data['text_message'];

      header("Cache-Control: public, must-revalidate");
      header("Pragma: no-cache");
      header("Content-Type: application/octet-stream");
      header("Content-Disposition: attachment; filename=search-" . time() . ".csv");
      header("Content-Transfer-Encoding: binary\n");

      $this->model_search_search->search_result_to_csv($this->request->get['cksum']);
   }


}

?>
