<?php


class ControllerQuarantineRemove extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/remove.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('user/user');

      $this->document->title = $this->data['text_quarantine'];


      $this->data['id'] = @$this->request->get['id'];


      $this->data['username'] = Registry::get('username');

      /* fix username if we are admin */

      if(isset($this->request->get['user']) && strlen($this->request->get['user']) > 1 && (Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->get['user']) == 1) ) {
         $this->data['username'] = $this->request->get['user'];
      }

      /* the same fix with POST requests */

      if(isset($this->request->post['user']) && strlen($this->request->post['user']) > 1 && (Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->post['user']) == 1) ) {
         $this->data['username'] = $this->request->post['user'];
      }


      $uid = $this->model_user_user->getUidByName($this->data['username']);
      $domain = $this->model_user_user->getDomainsByUid($uid);
      $my_q_dir = get_per_user_queue_dir($domain[0], $this->data['username'], $uid);


      /* purge selected messages */

      if($this->request->server['REQUEST_METHOD'] == 'POST' && isset($this->request->post['topurge'])){
         $n = 0;

         while(list($k, $v) = each($_POST)){
            $k = preg_replace("/_/", ".", $k);

            if($this->model_quarantine_message->checkId($k) && file_exists($my_q_dir . "/$k") && unlink($my_q_dir . "/$k") ){
               $n++;
            }
         }

         $this->data['x'] = $this->data['text_purged'] . " " . $n;
         
      }


      /* purge all messages */

      if($this->request->server['REQUEST_METHOD'] == 'GET' && isset($this->request->get['purgeallfromqueue'])){
         $n = 0;

         $files = scandir($my_q_dir, 1);

         foreach ($files as $file){
            if($this->model_quarantine_message->checkId($file) && file_exists($my_q_dir . "/$file") && unlink($my_q_dir . "/$file")){
               $n++;
            }
         }

         $this->data['x'] = $this->data['text_purged'] . " " . $n;
 
      }


      if($this->request->server['REQUEST_METHOD'] == 'GET') {
         if($this->model_quarantine_message->checkId($this->data['id']) && file_exists($my_q_dir . "/" . $this->data['id']) && unlink($my_q_dir . "/" . $this->data['id'])){
            $this->data['x'] = $this->data['text_successfully_removed'];
         } else {
            $this->data['x'] = $this->data['text_failed_to_remove'];
         }
      }


      $this->render();
   }


}

?>
