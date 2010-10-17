<?php


class ControllerUserWhitelist extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "user/whitelist.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');
      $language = Registry::get('language');

      $this->load->model('user/user');

      $this->document->title = $language->get('text_whitelist_settings');

      $this->data['text_cancel'] = $language->get('text_cancel');
      $this->data['text_submit'] = $language->get('text_submit');
      $this->data['text_back'] = $language->get('text_back');

      $__username = $_SESSION['username'];


      if($this->request->server['REQUEST_METHOD'] == 'POST'){
         $ret = $this->model_user_user->setWhitelist($__username, $this->request->post['whitelist']);

         if($ret == 1){
            $this->data['x'] = $language->get('text_successfully_updated');
         } else {
            $this->data['x'] = $language->get('text_failed_to_update');
         }
      }
      else {
         $this->data['whitelist'] = $this->model_user_user->getWhitelist($__username);
      }

      $this->render();
   }


}

?>
