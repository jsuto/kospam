<?php


class ControllerMessageView extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "message/view.tpl";
      $this->layout = "common/layout-empty";

      $session = Registry::get('session');
      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('search/search');
      $this->load->model('search/message');
      $this->load->model('audit/audit');

      $this->load->model('user/user');

      $this->document->title = $this->data['text_message'];

      $this->data['id'] = '';
      $this->data['search'] = '';
      $this->data['rcpt'] = array();

      if(isset($_SERVER['REQUEST_URI'])) { $a = preg_split("/\//", $_SERVER['REQUEST_URI']); $this->data['id'] = $a[count($a)-1]; }

      if($this->request->server['REQUEST_METHOD'] == 'POST') {
         $this->data['id'] = $this->request->post['id'];
         $this->data['search'] = $this->request->post['search'];
      }

      if(substr($this->data['id'], 0, 1) == 'a') {
         $this->template = "message/auto.tpl";
         $this->data['id'] = substr($this->data['id'], 1, 200);
      }


      if(!verify_piler_id($this->data['id'])) {
         AUDIT(ACTION_UNKNOWN, '', '', $this->data['id'], 'unknown id: ' . $this->data['id']);
         die("invalid id: " . $this->data['id']);
      }

      if(!$this->model_search_search->check_your_permission_by_id($this->data['id'])) {
         AUDIT(ACTION_UNAUTHORIZED_VIEW_MESSAGE, '', '', $this->data['id'], '');
         die("no permission for " . $this->data['id']);
      }


      AUDIT(ACTION_VIEW_MESSAGE, '', '', $this->data['id'], '');

      $this->data['username'] = Registry::get('username');

      if(Registry::get('auditor_user') == 1) { $this->data['rcpt'] = $this->model_search_search->get_message_addresses_in_my_domain($this->data['id']); }

      /* fix username if we are admin */

      if(isset($this->request->get['user']) && strlen($this->request->get['user']) > 1 && (Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->get['user']) == 1) ) {
         $this->data['username'] = $this->request->get['user'];
      }


      $this->data['clapf_id'] = $this->model_search_message->get_clapf_id_by_id($this->data['id']);

      $this->data['attachments'] = '';

      $this->data['message'] = $this->model_search_message->extract_message($this->data['clapf_id'], $this->data['search']);
      $this->data['spam'] = $this->model_search_message->is_message_spam($this->data['id']);

      $this->data['images'] = array();

      $this->render();
   }


}

?>
