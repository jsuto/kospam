<?php


class ControllerQuarantineHelper extends Controller {
   private $error = array();
   private $a = array(
                       'user' => '',
                       'from' => '',
                       'to' => '',
                       'subject' => '',
                       'date' => '',
                       'hamspam' => '',
                       'sort' => 'ts',
                       'order' => 1,
                       'page' => 0,
                       'page_len' => PAGE_LEN
                     );

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/helper.tpl";
      $this->layout = "common/layout-empty";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('quarantine/database');

      $this->load->model('user/user');

      $this->document->title = $this->data['text_quarantine'];


      $this->data['uid'] = 0;
      $this->data['uids'] = array();

      $this->data['username'] = Registry::get('username');


      if(isset($this->request->post['search']) && $this->request->post['search']) {
         $this->preprocess_search_request($this->request->post['search']);
      }
      else {
         $this->preprocess_simple_request();
      }

      $this->data['page_len'] = getPageLength();


      $this->data['page'] = 0;
      if(isset($this->request->post['page']) && $this->request->post['page'] > 0) { $this->data['page'] = $this->request->post['page']; }

      $this->a['page'] = $this->data['page'];

      if(SPAM_ONLY_QUARANTINE == 1) { $this->a['hamspam'] = "SPAM"; }

      if($this->a['to']) {
         $this->data['user'] = $this->a['user'] = $this->model_user_user->get_username_by_email($this->a['to']);

         if($this->a['user'] == "") {
            $this->template = "common/error.tpl";
            $this->data['errorstring'] = $this->data['text_non_existing_user'] . ": " . $this->a['to'];

            $this->render();
            exit;
         }
      }


      /* determine uids to query from quarantine database */

      if(Registry::get('admin_user') == 1) {
         if($this->a['user']) {
            array_push($this->data['uids'], $this->model_user_user->getUidByName($this->a['user']));
            $this->data['username'] = $this->a['user'];
         }
      }

      else if(Registry::get('domain_admin') == 1) {

         if($this->a['user'] && $this->model_user_user->isUserInMyDomain($this->request->post['user']) == 1) {
            array_push($this->data['uids'], $this->model_user_user->getUidByName($this->a['user']));
         }
         else {
            $this->data['uids'] = $this->model_user_user->getUidsByDomain($_SESSION['domain']);
         }
      }

      else {
         array_push($this->data['uids'], $this->model_user_user->getUidByName(Registry::get('username')));
      }


      /* sort and order */

      if(isset($this->request->post['order']) && ($this->request->post['order'] == "0" || $this->request->post['order'] == "1") ) { $this->a['order'] = $this->request->post['order']; }

      if(isset($this->request->post['sort'])) {
         if($this->request->post['sort'] == "ts") { $this->a['sort'] = "ts"; }
         if($this->request->post['sort'] == "from") { $this->a['sort'] = "from"; }
         if($this->request->post['sort'] == "to") { $this->a['sort'] = "to"; }
         if($this->request->post['sort'] == "subject") { $this->a['sort'] = "subject"; }
         if($this->request->post['sort'] == "size") { $this->a['sort'] = "size"; }
      }




      /* search terms */

      $uid = $this->model_user_user->getUidByName(Registry::get('username'));

      if($this->a['from'] || $this->a['to'] || $this->a['subject'] || $this->a['date']) {
         $this->model_quarantine_database->addSearchTerm($this->a, $uid);
      }



      /* get messages from quarantine */

      list ($this->data['n'], $this->data['total_size'], $this->data['messages']) =
                 $this->model_quarantine_database->getMessages($this->data['uids'], $this->a);

      if(count($this->data['messages']) == 0 && $this->a['page'] > 0) {
         $this->a['page']--;
         list ($this->data['n'], $this->data['total_size'], $this->data['messages']) = $this->model_quarantine_database->getMessages($this->data['uids'], $this->a);
      }


      /* paging info */

      $this->data['prev_page'] = $this->data['page'] - 1;
      $this->data['next_page'] = $this->data['page'] + 1;

      $this->data['total_pages'] = floor($this->data['n'] / $this->data['page_len']);

      $this->data['hits_from'] = $this->data['page'] * $this->data['page_len'] + 1;
      $this->data['hits_to'] = ($this->data['page']+1) * $this->data['page_len'];

      if($this->data['hits_to'] > $this->data['n']) { $this->data['hits_to'] = $this->data['n']; }



      $this->data['search'] = $this->a;

      $this->render();
   }


   private function preprocess_simple_request() {
      if(isset($this->request->post['from'])) { $this->a['from'] = $this->request->post['from']; }
      if(isset($this->request->post['to'])) { $this->a['to'] = $this->request->post['to']; }
      if(isset($this->request->post['subject'])) { $this->a['subject'] = $this->request->post['subject']; }
      if(isset($this->request->post['date'])) { $this->a['date'] = $this->request->post['date']; }
      if(isset($this->request->post['hamspam'])) { $this->a['hamspam'] = $this->request->post['hamspam']; }
   }


   private function preprocess_search_request($search = '') {
      $token = '';

      if($search == '') { return; }

      $s = preg_replace("/:/", ": ", $search);
      $s = preg_replace("/,/", " ", $s);
      $s = preg_replace("/\s{1,}/", " ", $s);
      $b = explode(" ", $s);

      while(list($k, $v) = each($b)) {

         if($v == '') { continue; }

         if($v == 'HAM' || $v == 'SPAM') { $this->a['hamspam'] = $v; continue; }
         else if($v == 'from:') { $token = 'from'; continue; }
         else if($v == 'to:') { $token = 'to'; continue; }
         else if($v == 'subject:') { $token = 'subject'; continue; }
         else if($v == 'date:') { $token = 'date'; continue; }
         else if($v == 'hamspam:') { $token = 'hamspam'; continue; }
         else {
            if(preg_match("/\d{4}\-\d{1,2}\-\d{1,2}/", $v)) {
               $this->a['date'] = $v; continue;
            }

            if(Registry::get('admin_user') == 0 && Registry::get('domain_admin') == 0 && strchr($v, '@')) { $this->a['from'] = $v; continue; }
         }

         if($token == 'from') { $this->a['from'] = $v; }
         else if($token == 'to') { $this->a['to'] = $v; }
         else if($token == 'date') { $this->a['date'] = $v; }
         else if($token == 'hamspam') { $this->a['hamspam'] = $v; }
         else {
            if($this->a['subject']) { $this->a['subject'] .= ' '; }
            $this->a['subject'] .= $v;
         }
      }

   }


}

?>
