<?php


class ControllerLoginLogin extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "login/login.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');

      $db = Registry::get('db');

      $this->load->model('user/auth');
      $this->load->model('user/prefs');

      //$this->document->title = $this->data['text_login'];

      if($this->request->server['REQUEST_METHOD'] == 'POST' && $this->validate() == true) {

         if($this->model_user_auth->checkLogin($this->request->post['username'], $_POST['password']) == 1) {

            $db_session = new DB("sqlite", "", "", "", SESSION_DATABASE, "");

            $this->model_user_prefs->getUserPrefs($db_session, $_SESSION['username']);

            LOGGER('logged in');

            if(isset($_POST['relocation']) && $_POST['relocation']) {
               header("Location: " . SITE_URL . "index.php?" . $_POST['relocation']);
            } else {
               header("Location: " . SITE_URL . "index.php?route=quarantine/quarantine");
            }

            exit;
         }

         $this->data['x'] = $this->data['text_invalid_email_or_password'];

      }


      $this->render();
   }


   private function validate() {

      if(strlen($this->request->post['username']) < 2){
         $this->error['username'] = $this->data['text_invalid_username'];
      }


      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }


}

?>
