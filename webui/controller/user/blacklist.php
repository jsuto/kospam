<?php


class ControllerUserBlacklist extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "user/blacklist.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');
      $language = Registry::get('language');

      if(DB_DRIVER == "ldap")
         $this->load->model('user/ldap/user');
      else
         $this->load->model('user/sql/user');

      $this->document->title = $language->get('text_blacklist_settings');

      $this->data['text_cancel'] = $language->get('text_cancel');
      $this->data['text_submit'] = $language->get('text_submit');
      $this->data['text_back'] = $language->get('text_back');

      if(DB_DRIVER == "ldap")
         $__username = $_SESSION['dn'];
      else
         $__username = $_SESSION['username'];

      if($this->request->server['REQUEST_METHOD'] == 'POST'){
         $ret = $this->model_user_user->setBlacklist($__username, $this->request->post['blacklist']);

         if($ret == 1){
            $this->data['x'] = $language->get('text_successfully_updated');
         } else {
            $this->data['x'] = $language->get('text_failed_to_update');
         }
      }
      else {
         $this->data['blacklist'] = $this->model_user_user->getBlacklist($__username);
      }

      $this->render();
   }


}

?>
