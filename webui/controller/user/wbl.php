<?php


class ControllerUserWbl extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "user/wbl.tpl";
      $this->layout = "common/layout";

      $request = Registry::get('request');
      $session = Registry::get('session');
      $db = Registry::get('db');
      $language = Registry::get('language');

      $this->load->model('user/wbl');

      $this->document->title = $language->get('text_whitelist_settings');
      $this->data['formurl'] = 'whitelist.php';

      if($this->request->get['type'] == 'black') {
         $this->document->title = $language->get('text_blacklist_settings');
         $this->data['formurl'] = 'blacklist.php';
      }

      if($this->request->server['REQUEST_METHOD'] == 'POST') {
         $this->data['type'] = $this->request->post['type'];

         if($this->validate() == true) {
            if($this->model_user_wbl->update($this->request->post['list'], $this->request->post['type']) == 1) {
               $this->data['x'] = $this->data['text_successfully_added'];
            } else {
               $this->data['errorstring'] = $this->data['text_failed_to_add'];
            }
         }
         else {
            $this->data['errorstring'] = $this->data['text_error_message'];
            $this->data['errors'] = $this->error;
            $this->data['post'] = $this->request->post;
         }
      }
      else {
         $this->data['type'] = $this->request->get['type'];
      }

      $this->data['list'] = $this->model_user_wbl->list($this->data['type']);

      $this->render();
   }


   private function validate() {
      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }

}

?>
