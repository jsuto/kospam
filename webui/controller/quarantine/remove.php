<?php


class ControllerQuarantineRemove extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/remove.tpl";
      $this->layout = "common/layout-empty";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('quarantine/database');

      $this->load->model('user/user');

      $this->document->title = $this->data['text_quarantine'];


      if(isset($this->request->get['id'])) { $this->data['id'] = $this->request->get['id']; }

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

      $Q = Registry::get('Q');

      if( (Registry::get('admin_user') == 1 || Registry::get('domain_admin') == 1) && (!isset($this->request->get['user']) || strlen($this->request->get['user']) < 1) ) {
         $this->data['username'] = '';
      }


      /* purge selected messages */

      if(isset($this->request->post['ids']) && isset($this->request->post['topurge'])){
         $n = 0;

         $ids = preg_split("/ /", $this->request->post['ids']);

         while(list($k, $v) = each($ids)){

            $a = preg_split("/\+/", $v);
            if(count($a) == 2) {
               $k = $a[0];

               $username = preg_replace("/\*/", ".", $a[1]);

               $uid = $this->model_user_user->getUidByName($username);
               $domain = $this->model_user_user->getDomainsByUid($uid);
               $my_q_dir = get_per_user_queue_dir($domain[0], $username, $uid);

               if($this->model_quarantine_message->checkId($k) && file_exists($my_q_dir . "/$k") && $this->model_quarantine_database->RemoveEntry($k, $uid) ){
                  if(REMOVE_FROM_QUARANTINE_WILL_UNLINK_FROM_FILESYSTEM == 1) { unlink($my_q_dir . "/$k"); }
                  $n++;
               }

            }

         }

         $this->data['message'] = $this->data['text_purged'] . " " . $n;
         
      }


      /* purge all messages */

      if(isset($this->request->get['purgeallfromqueue'])){
         $n = 0;

         if(REMOVE_FROM_QUARANTINE_WILL_UNLINK_FROM_FILESYSTEM == 1) {

            $files = scandir($my_q_dir, 1);

            foreach ($files as $file){
               if($this->model_quarantine_message->checkId($file) && file_exists($my_q_dir . "/$file") && $this->model_quarantine_database->RemoveEntry($file, $uid) ){
                  unlink($my_q_dir . "/$file");
               }
            }
         }

         $n = $this->model_quarantine_database->RemoveAllEntries($uid);

         $this->data['message'] = $this->data['text_purged'] . " " . $n;
 
      }


      /* purge one message */

      if(isset($this->request->get['id'])) {
         if($this->model_quarantine_message->checkId($this->data['id']) && file_exists($my_q_dir . "/" . $this->data['id']) ){
            $this->model_quarantine_database->RemoveEntry($this->data['id'], $uid);
            if(REMOVE_FROM_QUARANTINE_WILL_UNLINK_FROM_FILESYSTEM == 1) { unlink($my_q_dir . "/" . $this->data['id']); }
            $this->data['message'] = $this->data['text_successfully_removed'];
         } else {
            $this->data['message'] = $this->data['text_failed_to_remove'];
         }
      }


      $this->render();
   }


}

?>
